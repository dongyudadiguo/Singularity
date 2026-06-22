package main

import (
	"bufio"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"
	"strings"
	"time"
)

const hashSize = 32

var bootKey = [hashSize]byte{'C', 'V', 'M', '_', 'B', 'O', 'O', 'T'}

type tokenMap map[string][hashSize]byte

type builtBlock struct {
	name string
	body []byte
	hash [hashSize]byte
}

type graphEdge struct {
	parent [hashSize]byte
	child  [hashSize]byte
}

func main() {
	addr := flag.String("addr", "118.25.42.70:9000", "CVM server address")
	mapPath := flag.String("map", "mods_map.txt", "token map path")
	idPath := flag.String("id", "id.bin", "verified user id path")
	dryRun := flag.Bool("dry-run", false, "build and print block hashes without uploading")
	flag.Parse()

	toks, err := readTokenMap(*mapPath)
	must(err)

	blocks := buildBootEditorBlocks(toks)
	edges := buildBootEditorEdges(toks, blocks)
	chainHash := blocks[len(blocks)-1].hash
	printBlocks(blocks, edges, chainHash, toks.must("boot_run"))
	if *dryRun {
		return
	}

	user, err := readHashFile(*idPath)
	must(err)

	conn, err := net.DialTimeout("tcp", *addr, 10*time.Second)
	must(err)
	defer conn.Close()

	for _, b := range blocks {
		must(upload(conn, b.body))
	}
	for _, e := range edges {
		must(edge(conn, e.parent, e.child))
	}
	must(uset(conn, user, bootKey, chainHash))
	root := [hashSize]byte{}
	bootRun := toks.must("boot_run")
	must(edge(conn, root, bootRun))
	must(vote(conn, user, root, bootRun))
}

func printBlocks(blocks []builtBlock, edges []graphEdge, chainHash [hashSize]byte, bootRun [hashSize]byte) {
	for _, b := range blocks {
		fmt.Printf("block\t%s\t%x\n", b.name, b.hash)
	}
	for _, e := range edges {
		fmt.Printf("edge\t%x\t%x\n", e.parent, e.child)
	}
	fmt.Printf("boot_editor_hash\t%x\n", chainHash)
	fmt.Printf("root_token\tboot_run\t%x\n", bootRun)
}

func buildBootEditorBlocks(t tokenMap) []builtBlock {
	b := newBlockBuilder(t)

	b.block("fragment_round_rect_demo", func(c *chainBuilder) {
		c.rect(86, 86, 260, 140)
		c.pushColor(69, 130, 246)
		c.pushU64(28)
		c.add("surface_round_rect", nil)
		c.rect(86, 86, 260, 140)
		c.pushColor(224, 232, 255)
		c.pushU64(28)
		c.add("surface_round_frame", nil)
	})

	catalogRoot := b.block("catalog_root", func(c *chainBuilder) {
		c.text("CVM Token Catalog", 24, 24, 235, 238, 245)
		c.text("00 rounded rectangle fragment | 01 surface | 02 records | 03 graph | 04 payload | 05 state", 24, 54, 148, 163, 184)
	})
	b.block("catalog_surface_tokens", func(c *chainBuilder) {
		c.text("Surface tokens: open clear rect frame round_rect round_frame text poll pos", 24, 24, 148, 163, 184)
	})
	b.block("catalog_record_tokens", func(c *chainBuilder) {
		c.text("Record tokens: pack pack_hash pack_u64 at token_at payload_at insert replace delete", 24, 24, 148, 163, 184)
	})
	b.block("catalog_graph_tokens", func(c *chainBuilder) {
		c.text("Graph tokens: graph_children graph_child_at open_child view_push view_pop publish_view", 24, 24, 148, 163, 184)
	})
	b.block("catalog_payload_tokens", func(c *chainBuilder) {
		c.text("Payload tokens: payload_u64_le payload_hash32 payload_bytes bytes_empty", 24, 24, 148, 163, 184)
	})
	b.block("catalog_state_tokens", func(c *chainBuilder) {
		c.text("State tokens: state_hash_get/set state_index_get/set var_read var_write save_boot", 24, 24, 148, 163, 184)
	})

	initVars := b.block("init_boot_editor_vars", func(c *chainBuilder) {
		c.add("state_hash_get", nil)
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.pushHash(catalogRoot)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
		c.pushHash(t.must("noop"))
		c.varWrite("boot.browser.token")
		c.pushU64(0)
		c.varWrite("boot.editor.index")
	})

	insert := b.block("insert_selected_call", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.pushHash(t.must("call"))
		c.varRead("boot.browser.token")
		c.add("record_pack_hash", nil)
		c.add("records_insert", nil)
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
	})

	insertToken := b.block("insert_selected_token", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.varRead("boot.browser.token")
		c.add("bytes_empty", nil)
		c.add("record_pack", nil)
		c.add("records_insert", nil)
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
	})

	replace := b.block("replace_with_selected_call", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.pushHash(t.must("call"))
		c.varRead("boot.browser.token")
		c.add("record_pack_hash", nil)
		c.add("records_replace", nil)
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
	})

	replaceToken := b.block("replace_with_selected_token", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.varRead("boot.browser.token")
		c.add("bytes_empty", nil)
		c.add("record_pack", nil)
		c.add("records_replace", nil)
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
	})

	deleteRecord := b.block("delete_selected_record", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.add("records_delete", nil)
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
	})

	publishEdit := b.block("publish_edited_view", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
	})

	browserBack := b.block("browser_back", func(c *chainBuilder) {
		c.varRead("boot.browser.view")
		c.add("state_hash_set", nil)
		c.add("view_pop", nil)
		c.add("state_hash_get", nil)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
	})

	selectors := make([][hashSize]byte, 8)
	for i := range selectors {
		idx := uint64(i)
		selectors[i] = b.block(fmt.Sprintf("select_row_%d", i), func(c *chainBuilder) {
			c.pushU64(idx)
			c.varWrite("boot.editor.index")
		})
	}

	browserSelectors := make([][hashSize]byte, 8)
	for i := range browserSelectors {
		idx := uint64(i)
		browserSelectors[i] = b.block(fmt.Sprintf("browse_child_%d", i), func(c *chainBuilder) {
			c.varRead("boot.browser.view")
			c.add("state_hash_set", nil)
			c.pushU64(idx)
			c.add("state_index_set", nil)
			c.add("open_child", nil)
			c.add("state_hash_get", nil)
			c.add("dup", nil)
			c.varWrite("boot.browser.view")
			c.varWrite("boot.browser.token")
		})
	}

	mouse := b.block("mouse_dispatch", func(c *chainBuilder) {
		for i, target := range selectors {
			c.rectContains(20, uint64(220+i*34), 590, 30)
			c.add("call_cond_static", target[:])
		}
		for i, target := range browserSelectors {
			c.rectContains(660, uint64(220+i*34), 580, 30)
			c.add("call_cond_static", target[:])
		}
		c.rectContains(20, 560, 180, 40)
		c.add("call_cond_static", insert[:])
		c.rectContains(220, 560, 170, 40)
		c.add("call_cond_static", insertToken[:])
		c.rectContains(410, 560, 170, 40)
		c.add("call_cond_static", replace[:])
		c.rectContains(20, 608, 170, 32)
		c.add("call_cond_static", replaceToken[:])
		c.rectContains(210, 608, 150, 32)
		c.add("call_cond_static", deleteRecord[:])
		c.rectContains(660, 560, 130, 40)
		c.add("call_cond_static", browserBack[:])
		c.rectContains(810, 560, 150, 40)
		c.add("call_cond_static", publishEdit[:])
	})

	b.block("boot_editor_entry", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.add("zero", nil)
		c.add("eq", nil)
		c.add("call_cond_static", initVars[:])

		c.varRead("boot.browser.view")
		c.add("state_hash_set", nil)

		c.pushU64(1280)
		c.pushU64(640)
		c.add("surface_open", nil)
		c.add("pop", nil)

		c.pushColor(18, 20, 28)
		c.add("surface_clear", nil)

		c.rect(0, 0, 1280, 56)
		c.pushColor(36, 41, 58)
		c.add("surface_rect", nil)
		c.text("CVM Boot Editor", 24, 18, 235, 238, 245)
		c.text("browse the published token catalog on the right, edit/publish the boot block on the left", 24, 78, 148, 163, 184)
		c.text("edit hash:", 24, 116, 137, 180, 250)
		c.varRead("boot.editor.view")
		c.add("hash_hex", nil)
		c.pushU64(118)
		c.pushU64(116)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("edit index:", 24, 156, 137, 180, 250)
		c.varRead("boot.editor.index")
		c.add("hash_hex", nil)
		c.pushU64(128)
		c.pushU64(156)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("edit records 0..7", 24, 194, 170, 178, 196)
		c.text("browser view:", 660, 116, 137, 180, 250)
		c.varRead("boot.browser.view")
		c.add("hash_hex", nil)
		c.pushU64(776)
		c.pushU64(116)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("selected token:", 660, 156, 137, 180, 250)
		c.varRead("boot.browser.token")
		c.add("hash_hex", nil)
		c.pushU64(794)
		c.pushU64(156)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("network children 0..7", 660, 194, 170, 178, 196)

		for i := 0; i < 8; i++ {
			y := uint64(220 + i*34)
			c.rect(20, y, 590, 30)
			c.pushColor(28, 32, 44)
			c.add("surface_rect", nil)
			c.rect(20, y, 590, 30)
			c.pushColor(58, 68, 92)
			c.add("surface_frame", nil)
			c.text(fmt.Sprintf("%02d", i), 32, y+8, 137, 180, 250)
			c.varRead("boot.editor.view")
			c.pushU64(uint64(i))
			c.add("records_at", nil)
			c.pushU64(0)
			c.add("record_token_at", nil)
			c.add("hash_hex", nil)
			c.pushU64(70)
			c.pushU64(y + 8)
			c.pushColor(218, 224, 235)
			c.add("surface_text", nil)
		}

		for i := 0; i < 8; i++ {
			y := uint64(220 + i*34)
			c.rect(660, y, 580, 30)
			c.pushColor(24, 34, 35)
			c.add("surface_rect", nil)
			c.rect(660, y, 580, 30)
			c.pushColor(58, 92, 88)
			c.add("surface_frame", nil)
			c.text(fmt.Sprintf("%02d", i), 672, y+8, 116, 211, 194)
			c.varRead("boot.browser.view")
			c.pushU64(uint64(i))
			c.add("graph_child_at", nil)
			c.add("hash_hex", nil)
			c.pushU64(710)
			c.pushU64(y + 8)
			c.pushColor(218, 224, 235)
			c.add("surface_text", nil)
		}

		c.button("Insert call", 20, 560, 180, 40, 43, 116, 78)
		c.button("Insert token", 220, 560, 170, 40, 56, 110, 129)
		c.button("Replace call", 410, 560, 170, 40, 116, 94, 43)
		c.button("Replace token", 20, 608, 170, 32, 92, 84, 132)
		c.button("Delete", 210, 608, 150, 32, 129, 63, 63)
		c.button("Back", 660, 560, 130, 40, 47, 94, 117)
		c.button("Publish boot", 810, 560, 150, 40, 67, 120, 86)
		c.text("Catalog: 00 rounded-rect fragment, 01 surface, 02 records, 03 graph, 04 payload, 05 state.", 980, 572, 148, 163, 184)

		c.add("surface_poll", nil)
		c.pushU64(513)
		c.add("eq", nil)
		c.add("call_cond_static", mouse[:])
		c.add("surface_event_clear", nil)
		c.pushU64(33)
		c.add("sleep_ms", nil)
	})

	return b.finish()
}

func buildBootEditorEdges(t tokenMap, blocks []builtBlock) []graphEdge {
	byName := map[string][hashSize]byte{}
	for _, blk := range blocks {
		byName[blk.name] = blk.hash
	}
	root := byName["catalog_root"]

	edges := []graphEdge{
		{root, byName["fragment_round_rect_demo"]},
		{root, byName["catalog_surface_tokens"]},
		{root, byName["catalog_record_tokens"]},
		{root, byName["catalog_graph_tokens"]},
		{root, byName["catalog_payload_tokens"]},
		{root, byName["catalog_state_tokens"]},
	}

	tokenEdge := func(cat string, names ...string) {
		catHash := byName[cat]
		for _, n := range names {
			if tok, ok := t[n]; ok {
				edges = append(edges, graphEdge{catHash, tok})
			}
		}
	}

	tokenEdge("catalog_surface_tokens",
		"surface_open", "surface_clear", "surface_rect", "surface_frame",
		"surface_round_rect", "surface_round_frame", "surface_text", "surface_poll",
	)

	tokenEdge("catalog_record_tokens",
		"record_pack", "record_pack_hash", "record_pack_u64",
		"records_insert", "records_replace", "records_delete",
	)

	tokenEdge("catalog_graph_tokens",
		"graph_children", "graph_child_at", "open_child",
		"view_push", "view_pop", "publish_view",
	)

	tokenEdge("catalog_payload_tokens",
		"payload_u64_le", "payload_hash32", "payload_bytes", "bytes_empty",
	)

	tokenEdge("catalog_state_tokens",
		"state_hash_get", "state_hash_set", "state_index_get", "state_index_set",
		"var_read", "var_write", "save_boot",
	)

	return edges
}

type blockBuilder struct {
	tokens tokenMap
	blocks []builtBlock
}

func newBlockBuilder(t tokenMap) *blockBuilder { return &blockBuilder{tokens: t} }

func (b *blockBuilder) block(name string, fn func(*chainBuilder)) [hashSize]byte {
	c := &chainBuilder{tokens: b.tokens}
	fn(c)
	c.end()
	h := sha256.Sum256(c.out)
	b.blocks = append(b.blocks, builtBlock{name: name, body: c.out, hash: h})
	return h
}

func (b *blockBuilder) finish() []builtBlock {
	return b.blocks
}

type chainBuilder struct {
	tokens tokenMap
	out    []byte
}

func (c *chainBuilder) add(name string, payload []byte) {
	c.out = append(c.out, record(c.tokens.must(name), payload)...)
}

func (c *chainBuilder) pushU64(v uint64) { c.add("payload_u64_le", u64(v)) }

func (c *chainBuilder) pushHash(h [hashSize]byte) { c.add("payload_hash32", h[:]) }

func (c *chainBuilder) varRead(name string) { c.add("var_read", []byte(name)) }

func (c *chainBuilder) varWrite(name string) { c.add("var_write", []byte(name)) }

func (c *chainBuilder) pushColor(r, g, b uint64) {
	c.pushU64(r)
	c.pushU64(g)
	c.pushU64(b)
	c.add("color_rgb", nil)
}

func (c *chainBuilder) rect(x, y, w, h uint64) {
	c.pushU64(x)
	c.pushU64(y)
	c.pushU64(w)
	c.pushU64(h)
	c.add("rect_make", nil)
}

func (c *chainBuilder) text(s string, x, y uint64, r, g, b uint64) {
	c.add("payload_bytes", []byte(s))
	c.pushU64(x)
	c.pushU64(y)
	c.pushColor(r, g, b)
	c.add("surface_text", nil)
}

func (c *chainBuilder) button(label string, x, y, w, h uint64, r, g, b uint64) {
	c.rect(x, y, w, h)
	c.pushColor(r, g, b)
	c.pushU64(14)
	c.add("surface_round_rect", nil)
	c.rect(x, y, w, h)
	c.pushColor(180, 190, 210)
	c.pushU64(14)
	c.add("surface_round_frame", nil)
	c.text(label, x+14, y+13, 245, 247, 250)
}

func (c *chainBuilder) rectContains(x, y, w, h uint64) {
	c.rect(x, y, w, h)
	c.add("surface_pos", nil)
	c.add("pair_first", nil)
	c.add("surface_pos", nil)
	c.add("pair_second", nil)
	c.add("rect_contains", nil)
}

func (c *chainBuilder) end() { c.out = append(c.out, make([]byte, hashSize)...) }

func u64(v uint64) []byte {
	b := make([]byte, 8)
	binary.LittleEndian.PutUint64(b, v)
	return b
}

func record(tok [hashSize]byte, payload []byte) []byte {
	out := make([]byte, 36+len(payload))
	copy(out, tok[:])
	binary.LittleEndian.PutUint32(out[32:36], uint32(len(payload)+4))
	copy(out[36:], payload)
	return out
}

func readTokenMap(path string) (tokenMap, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	out := tokenMap{}
	s := bufio.NewScanner(f)
	for s.Scan() {
		parts := strings.Split(s.Text(), "\t")
		if len(parts) < 2 {
			continue
		}
		raw, err := hex.DecodeString(parts[1])
		if err != nil || len(raw) != hashSize {
			continue
		}
		var h [hashSize]byte
		copy(h[:], raw)
		out[parts[0]] = h
	}
	if err := s.Err(); err != nil {
		return nil, err
	}
	return out, nil
}

func (m tokenMap) must(name string) [hashSize]byte {
	h, ok := m[name]
	if !ok {
		panic("missing token: " + name)
	}
	return h
}

func readHashFile(path string) ([hashSize]byte, error) {
	var h [hashSize]byte
	b, err := os.ReadFile(filepath.Clean(path))
	if err != nil {
		return h, err
	}
	if len(b) != hashSize {
		return h, fmt.Errorf("%s must be 32 bytes, got %d", path, len(b))
	}
	copy(h[:], b)
	return h, nil
}

func upload(c net.Conn, body []byte) error {
	status, out, err := request(c, 2, body)
	if err != nil {
		return err
	}
	if status != 0 {
		return fmt.Errorf("upload status %d", status)
	}
	sum := sha256.Sum256(body)
	if len(out) != hashSize || string(out) != string(sum[:]) {
		return errors.New("upload hash mismatch")
	}
	return nil
}

func edge(c net.Conn, parent, child [hashSize]byte) error {
	body := append(parent[:], child[:]...)
	status, _, err := request(c, 4, body)
	if err != nil {
		return err
	}
	if status != 0 {
		return fmt.Errorf("edge status %d", status)
	}
	return nil
}

func uset(c net.Conn, user, key, val [hashSize]byte) error {
	body := make([]byte, 0, 96)
	body = append(body, user[:]...)
	body = append(body, key[:]...)
	body = append(body, val[:]...)
	status, _, err := request(c, 7, body)
	if err != nil {
		return err
	}
	if status != 0 {
		return fmt.Errorf("uset status %d", status)
	}
	return nil
}

func vote(c net.Conn, user, parent, child [hashSize]byte) error {
	body := make([]byte, 0, 96)
	body = append(body, user[:]...)
	body = append(body, parent[:]...)
	body = append(body, child[:]...)
	status, _, err := request(c, 6, body)
	if err != nil {
		return err
	}
	if status != 0 {
		return fmt.Errorf("vote status %d", status)
	}
	return nil
}

func request(c net.Conn, op byte, body []byte) (byte, []byte, error) {
	var h [5]byte
	h[0] = op
	binary.BigEndian.PutUint32(h[1:5], uint32(len(body)))
	if _, err := c.Write(h[:]); err != nil {
		return 0, nil, err
	}
	if len(body) != 0 {
		if _, err := c.Write(body); err != nil {
			return 0, nil, err
		}
	}
	if _, err := io.ReadFull(c, h[:]); err != nil {
		return 0, nil, err
	}
	n := binary.BigEndian.Uint32(h[1:5])
	out := make([]byte, n)
	if n != 0 {
		if _, err := io.ReadFull(c, out); err != nil {
			return 0, nil, err
		}
	}
	return h[0], out, nil
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
