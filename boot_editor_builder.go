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
	rootHash := [hashSize]byte{}

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
	b.block("fragment_text_badge_demo", func(c *chainBuilder) {
		c.rect(88, 92, 300, 82)
		c.pushColor(88, 80, 236)
		c.pushU64(22)
		c.add("surface_round_rect", nil)
		c.text("starter badge", 116, 118, 245, 247, 250)
		c.text("play me, then append me", 116, 146, 203, 213, 225)
	})
	b.block("fragment_color_shape_demo", func(c *chainBuilder) {
		c.rect(86, 86, 112, 112)
		c.pushColor(16, 185, 129)
		c.add("surface_rect", nil)
		c.rect(230, 86, 112, 112)
		c.pushColor(245, 158, 11)
		c.pushU64(26)
		c.add("surface_round_rect", nil)
		c.rect(374, 86, 112, 112)
		c.pushColor(244, 63, 94)
		c.add("surface_frame", nil)
	})

	catalogRoot := b.block("catalog_root", func(c *chainBuilder) {
		c.text("CVM Token Catalog", 24, 24, 235, 238, 245)
		c.text("00 rounded rectangle | 01 text badge | 02 color shape | 03 surface | 04 records | 05 graph | 06 payload | 07 state", 24, 54, 148, 163, 184)
	})
	b.block("catalog_surface_tokens", func(c *chainBuilder) {
		c.text("Surface tokens: open clear rect frame round_rect round_frame text poll pos size event_clear", 24, 24, 148, 163, 184)
	})
	b.block("catalog_record_tokens", func(c *chainBuilder) {
		c.text("Record tokens: pack pack_empty pack_hash at token_at payload_at insert replace delete count valid", 24, 24, 148, 163, 184)
	})
	b.block("catalog_graph_tokens", func(c *chainBuilder) {
		c.text("Graph tokens: graph_children graph_child_at open_child view_push view_pop publish_view link", 24, 24, 148, 163, 184)
	})
	b.block("catalog_payload_tokens", func(c *chainBuilder) {
		c.text("Payload tokens: payload_u64_le payload_hash32 payload_bytes bytes_empty hash_hex u64_dec_bytes", 24, 24, 148, 163, 184)
	})
	b.block("catalog_state_tokens", func(c *chainBuilder) {
		c.text("State/control tokens: state_hash_get/set state_index_get/set var_read var_write save_boot call call_stack", 24, 24, 148, 163, 184)
	})

	saveEditedView := func(c *chainBuilder) {
		c.add("dup", nil)
		c.varWrite("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
	}

	setDirty := func(c *chainBuilder) {
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
	}

	initVars := b.block("init_boot_editor_vars", func(c *chainBuilder) {
		c.add("state_hash_get", nil)
		c.varWrite("boot.editor.view")
		c.pushHash(rootHash)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
		c.pushU64(0)
		c.varWrite("boot.editor.index")
		setDirty(c)
	})

	wrapSelectedToken := b.block("wrap_selected_token", func(c *chainBuilder) {
		c.add("records_empty", nil)
		c.pushU64(0)
		c.varRead("boot.browser.token")
		c.add("record_pack_empty", nil)
		c.add("records_insert", nil)
	})

	wrapSelectedTokenStore := b.block("wrap_selected_token_store", func(c *chainBuilder) {
		c.add("call", wrapSelectedToken[:])
		c.add("dup", nil)
		c.varWrite("boot.browser.token")
	})

	browserRoot := b.block("browser_root", func(c *chainBuilder) {
		c.pushHash(rootHash)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
	})

	browserCatalog := b.block("browser_catalog", func(c *chainBuilder) {
		c.pushHash(catalogRoot)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
	})

	insert := b.block("insert_selected_call", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.pushHash(t.must("call"))
		c.varRead("boot.browser.token")
		c.add("record_pack_hash", nil)
		c.add("records_insert", nil)
		saveEditedView(c)
	})

	insertToken := b.block("insert_selected_token", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.varRead("boot.browser.token")
		c.add("bytes_empty", nil)
		c.add("record_pack", nil)
		c.add("records_insert", nil)
		saveEditedView(c)
	})

	replace := b.block("replace_with_selected_call", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.pushHash(t.must("call"))
		c.varRead("boot.browser.token")
		c.add("record_pack_hash", nil)
		c.add("records_replace", nil)
		saveEditedView(c)
	})

	replaceToken := b.block("replace_with_selected_token", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.varRead("boot.browser.token")
		c.add("bytes_empty", nil)
		c.add("record_pack", nil)
		c.add("records_replace", nil)
		saveEditedView(c)
	})

	deleteRecord := b.block("delete_selected_record", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.varRead("boot.editor.index")
		c.add("records_delete", nil)
		saveEditedView(c)
	})

	appendSelectedCall := b.block("append_selected_call", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.add("dup", nil)
		c.add("records_count", nil)
		c.pushHash(t.must("call"))
		c.varRead("boot.browser.token")
		c.add("record_pack_hash", nil)
		c.add("records_insert", nil)
		saveEditedView(c)
	})

	appendSelectedToken := b.block("append_selected_token", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.add("dup", nil)
		c.add("records_count", nil)
		c.varRead("boot.browser.token")
		c.add("record_pack_empty", nil)
		c.add("records_insert", nil)
		saveEditedView(c)
	})

	appendSelected := b.block("append_selected", func(c *chainBuilder) {
		c.varRead("boot.browser.token")
		c.add("records_valid", nil)
		c.add("dup", nil)
		c.varWrite("boot.browser.is_block")
		c.add("call_cond_static", appendSelectedCall[:])
		c.varRead("boot.browser.is_block")
		c.add("not", nil)
		c.add("call_cond_static", appendSelectedToken[:])
	})

	playSelectedBlock := b.block("play_selected_block", func(c *chainBuilder) {
		c.varRead("boot.browser.token")
		c.add("call_stack", nil)
		c.add("pop", nil)
		setDirty(c)
	})

	playSelectedToken := b.block("play_selected_token", func(c *chainBuilder) {
		c.add("call", wrapSelectedToken[:])
		c.add("call_stack", nil)
		c.add("pop", nil)
		setDirty(c)
	})

	playSelected := b.block("play_selected", func(c *chainBuilder) {
		c.varRead("boot.browser.token")
		c.add("records_valid", nil)
		c.add("dup", nil)
		c.varWrite("boot.browser.is_block")
		c.add("call_cond_static", playSelectedBlock[:])
		c.varRead("boot.browser.is_block")
		c.add("not", nil)
		c.add("call_cond_static", playSelectedToken[:])
	})

	publishSelectedBlock := b.block("publish_selected_block", func(c *chainBuilder) {
		c.varRead("boot.browser.token")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		setDirty(c)
	})

	publishSelectedToken := b.block("publish_selected_token", func(c *chainBuilder) {
		c.add("call", wrapSelectedTokenStore[:])
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		setDirty(c)
	})

	publishSelected := b.block("publish_selected", func(c *chainBuilder) {
		c.varRead("boot.browser.token")
		c.add("records_valid", nil)
		c.add("dup", nil)
		c.varWrite("boot.browser.is_block")
		c.add("call_cond_static", publishSelectedBlock[:])
		c.varRead("boot.browser.is_block")
		c.add("not", nil)
		c.add("call_cond_static", publishSelectedToken[:])
	})

	publishEdit := b.block("publish_edited_view", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("save_boot", nil)
		setDirty(c)
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
		c.rectContains(20, 540, 190, 40)
		c.add("call_cond_static", appendSelected[:])
		c.rectContains(225, 540, 170, 40)
		c.add("call_cond_static", insert[:])
		c.rectContains(410, 540, 170, 40)
		c.add("call_cond_static", insertToken[:])
		c.rectContains(20, 588, 170, 32)
		c.add("call_cond_static", replace[:])
		c.rectContains(205, 588, 185, 32)
		c.add("call_cond_static", replaceToken[:])
		c.rectContains(405, 588, 175, 32)
		c.add("call_cond_static", deleteRecord[:])
		c.rectContains(20, 632, 180, 40)
		c.add("call_cond_static", publishEdit[:])
		c.rectContains(660, 540, 100, 40)
		c.add("call_cond_static", browserRoot[:])
		c.rectContains(780, 540, 130, 40)
		c.add("call_cond_static", browserCatalog[:])
		c.rectContains(930, 540, 100, 40)
		c.add("call_cond_static", browserBack[:])
		c.rectContains(660, 588, 170, 40)
		c.add("call_cond_static", playSelected[:])
		c.rectContains(850, 588, 190, 40)
		c.add("call_cond_static", publishSelected[:])
	})

	draw := b.block("boot_editor_draw", func(c *chainBuilder) {
		c.varRead("boot.browser.view")
		c.add("state_hash_set", nil)

		c.pushColor(18, 20, 28)
		c.add("surface_clear", nil)

		c.rect(0, 0, 1280, 56)
		c.pushColor(36, 41, 58)
		c.add("surface_rect", nil)
		c.text("Browse -> Play -> Add -> Publish", 24, 18, 235, 238, 245)
		c.text("right side starts at public root; Catalog opens generated tokens and starter fragments", 24, 78, 148, 163, 184)
		c.text("edit hash:", 24, 110, 137, 180, 250)
		c.varRead("boot.editor.view")
		c.add("hash_hex", nil)
		c.pushU64(118)
		c.pushU64(110)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("edit index:", 24, 148, 137, 180, 250)
		c.varRead("boot.editor.index")
		c.add("u64_dec_bytes", nil)
		c.pushU64(128)
		c.pushU64(148)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("edit count:", 210, 148, 137, 180, 250)
		c.varRead("boot.editor.view")
		c.add("records_count", nil)
		c.add("u64_dec_bytes", nil)
		c.pushU64(318)
		c.pushU64(148)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("edit records 0..7", 24, 194, 170, 178, 196)
		c.text("public root:", 660, 100, 137, 180, 250)
		c.pushHash(rootHash)
		c.add("hash_hex", nil)
		c.pushU64(780)
		c.pushU64(100)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("browser view:", 660, 132, 137, 180, 250)
		c.varRead("boot.browser.view")
		c.add("hash_hex", nil)
		c.pushU64(776)
		c.pushU64(132)
		c.pushColor(218, 224, 235)
		c.add("surface_text", nil)
		c.text("selected hash:", 660, 164, 137, 180, 250)
		c.varRead("boot.browser.token")
		c.add("hash_hex", nil)
		c.pushU64(794)
		c.pushU64(164)
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

		c.button("Append selected", 20, 540, 190, 40, 43, 116, 78)
		c.button("Insert call", 225, 540, 170, 40, 43, 116, 78)
		c.button("Insert token", 410, 540, 170, 40, 56, 110, 129)
		c.button("Replace call", 20, 588, 170, 32, 116, 94, 43)
		c.button("Replace token", 205, 588, 185, 32, 92, 84, 132)
		c.button("Delete", 405, 588, 175, 32, 129, 63, 63)
		c.button("Publish edit", 20, 632, 180, 40, 67, 120, 86)
		c.button("Root", 660, 540, 100, 40, 47, 94, 117)
		c.button("Catalog", 780, 540, 130, 40, 47, 94, 117)
		c.button("Back", 930, 540, 100, 40, 47, 94, 117)
		c.button("Play selected", 660, 588, 170, 40, 72, 92, 170)
		c.button("Publish selected", 850, 588, 190, 40, 67, 120, 86)
		c.text("Catalog rows: 00 rounded, 01 badge, 02 shapes, 03 surface, 04 records, 05 graph, 06 payload, 07 state.", 660, 650, 148, 163, 184)
	})

	drawDirty := b.block("boot_editor_draw_dirty", func(c *chainBuilder) {
		c.add("call", draw[:])
		c.pushU64(0)
		c.varWrite("boot.editor.dirty")
	})

	markDirty := b.block("boot_editor_mark_dirty", func(c *chainBuilder) {
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
	})

	frameLoop := b.block("boot_editor_frame_loop", func(c *chainBuilder) {
		c.varRead("boot.editor.dirty")
		c.add("call_cond_static", drawDirty[:])
		c.add("surface_poll", nil)
		c.add("dup", nil)
		c.pushU64(513)
		c.add("eq", nil)
		c.add("call_cond_static", mouse[:])
		c.add("dup", nil)
		c.pushU64(513)
		c.add("eq", nil)
		c.add("call_cond_static", markDirty[:])
		c.pushU64(0xffffffff)
		c.add("ne", nil)
		c.add("surface_event_clear", nil)
		c.pushU64(33)
		c.add("sleep_ms", nil)
		c.add("again_cond", nil)
	})

	b.block("boot_editor_entry", func(c *chainBuilder) {
		c.varRead("boot.editor.view")
		c.add("zero", nil)
		c.add("eq", nil)
		c.add("call_cond_static", initVars[:])
		c.pushU64(1280)
		c.pushU64(720)
		c.add("surface_open", nil)
		c.add("pop", nil)
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
		c.add("call", frameLoop[:])
		c.add("surface_close", nil)
	})

	return b.finish()
}

func buildBootEditorEdges(t tokenMap, blocks []builtBlock) []graphEdge {
	byName := map[string][hashSize]byte{}
	for _, blk := range blocks {
		byName[blk.name] = blk.hash
	}
	publicRoot := [hashSize]byte{}
	catalogRoot := byName["catalog_root"]

	edges := []graphEdge{
		{publicRoot, catalogRoot},
		{catalogRoot, byName["fragment_round_rect_demo"]},
		{catalogRoot, byName["fragment_text_badge_demo"]},
		{catalogRoot, byName["fragment_color_shape_demo"]},
		{catalogRoot, byName["catalog_surface_tokens"]},
		{catalogRoot, byName["catalog_record_tokens"]},
		{catalogRoot, byName["catalog_graph_tokens"]},
		{catalogRoot, byName["catalog_payload_tokens"]},
		{catalogRoot, byName["catalog_state_tokens"]},
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
		"surface_pos", "surface_size", "surface_event_clear", "sleep_ms",
	)

	tokenEdge("catalog_record_tokens",
		"record_pack", "record_pack_empty", "record_pack_hash", "record_pack_u64",
		"records_empty", "records_insert", "records_replace", "records_delete",
		"records_count", "records_valid", "records_at", "record_token_at", "record_payload_at",
	)

	tokenEdge("catalog_graph_tokens",
		"graph_children", "graph_child_at", "open_child",
		"view_push", "view_pop", "publish_view", "graph_link", "graph_link_root",
	)

	tokenEdge("catalog_payload_tokens",
		"payload_u64_le", "payload_hash32", "payload_bytes", "bytes_empty",
		"hash_hex", "u64_dec_bytes",
	)

	tokenEdge("catalog_state_tokens",
		"state_hash_get", "state_hash_set", "state_index_get", "state_index_set",
		"var_read", "var_write", "save_boot", "zero", "dup", "pop", "not",
		"eq", "ne", "call", "call_cond_static", "call_stack", "again_cond",
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
