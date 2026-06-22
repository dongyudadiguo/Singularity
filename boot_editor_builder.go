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

	previousBoot, found, err := uget(conn, user, bootKey)
	must(err)
	if found {
		fmt.Printf("previous_boot_hash\t%x\n", previousBoot)
	} else {
		fmt.Printf("previous_boot_hash\t<none>\n")
	}

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
	invalidIndex := uint64(^uint64(0))
	toyShelfKey := fixedHash("CVM_TOY_SHELF")
	emptyHash := sha256.Sum256(nil)

	toyTitleBytes := b.rawBlock("toy_title_bytes", []byte("我的第一个玩具"))
	toyMessageReady := b.rawBlock("toy_message_ready", []byte("点一个玩具，摆弄一下，觉得有意思就发布。"))
	toyMessageEditText := b.rawBlock("toy_message_edit_text", []byte("正在编辑文字：输入中文，Backspace 删除，点 完成 返回。"))
	toyMessagePublished := b.rawBlock("toy_message_published", []byte("已发布到玩具流，没有改启动。"))
	toyMessageBoot := b.rawBlock("toy_message_boot", []byte("已设为启动玩具。"))
	toyStickerText := b.rawBlock("toy_sticker_text", []byte("你好，玩具世界"))
	toyBadgeText := b.rawBlock("toy_badge_text", []byte("闪亮徽章"))
	toyPulseText := b.rawBlock("toy_pulse_text", []byte("会呼吸"))
	zeroHash := [hashSize]byte{}
	shapePayload := toyObjectPayload(1, 55, 62, 140, 96, toyColor(15, 185, 129), 0, 0, zeroHash, zeroHash)
	roundPayload := toyObjectPayload(2, 238, 72, 160, 112, toyColor(99, 102, 241), 28, 0, zeroHash, zeroHash)
	framePayload := toyObjectPayload(3, 446, 80, 150, 104, toyColor(244, 114, 182), 0, 0, zeroHash, zeroHash)
	stickerPayload := toyObjectPayload(4, 78, 232, 260, 48, toyColor(255, 255, 255), 0, 0, toyStickerText, zeroHash)
	badgePayload := toyObjectPayload(5, 382, 226, 210, 116, toyColor(245, 158, 11), 24, 0, toyBadgeText, zeroHash)
	animatedPayload := toyObjectPayload(7, 198, 372, 220, 80, toyColor(56, 189, 248), 32, 0, toyPulseText, zeroHash)
	initialStageData := b.rawBlock("toy_initial_stage_data", toyStageBlock(t.must("noop"), shapePayload, roundPayload, framePayload, stickerPayload, badgePayload, animatedPayload))
	shapeRecord := b.rawBlock("toy_record_shape", record(t.must("noop"), toyObjectPayload(1, 88, 90, 140, 96, toyColor(34, 197, 94), 0, 0, zeroHash, zeroHash)))
	roundRecord := b.rawBlock("toy_record_round", record(t.must("noop"), toyObjectPayload(2, 120, 120, 160, 110, toyColor(99, 102, 241), 30, 0, zeroHash, zeroHash)))
	frameRecord := b.rawBlock("toy_record_frame", record(t.must("noop"), toyObjectPayload(3, 150, 150, 180, 120, toyColor(236, 72, 153), 0, 0, zeroHash, zeroHash)))
	stickerRecord := b.rawBlock("toy_record_sticker", record(t.must("noop"), toyObjectPayload(4, 160, 205, 260, 48, toyColor(255, 255, 255), 0, 0, toyStickerText, zeroHash)))
	badgeRecord := b.rawBlock("toy_record_badge", record(t.must("noop"), toyObjectPayload(5, 220, 240, 220, 120, toyColor(245, 158, 11), 28, 0, toyBadgeText, zeroHash)))
	sparkleRecord := b.rawBlock("toy_record_sparkle", record(t.must("noop"), toyObjectPayload(1, 265, 180, 54, 54, toyColor(250, 204, 21), 0, 0, zeroHash, zeroHash)))
	animatedRecord := b.rawBlock("toy_record_animated", record(t.must("noop"), toyObjectPayload(7, 210, 330, 220, 80, toyColor(56, 189, 248), 32, 0, toyPulseText, zeroHash)))

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
	toyGalleryRoot := b.block("toy_gallery_root", func(c *chainBuilder) {
		c.pushColor(19, 26, 42)
		c.add("surface_clear", nil)
		c.textUTF8("玩具流", 32, 28, 245, 247, 250)
		c.textUTF8("这里收集可播放、可加到舞台、可重新发布的玩具包。", 32, 68, 148, 163, 184)
	})
	refreshBootBrowser := b.block("refresh_boot_browser_children", func(c *chainBuilder) {
		c.varRead("boot.browser.view")
		c.add("graph_children", nil)
		c.varWrite("boot.browser.children")
	})
	refreshToyBrowser := b.block("refresh_toy_browser_children", func(c *chainBuilder) {
		c.varRead("toy.browser.view")
		c.add("graph_children", nil)
		c.varWrite("toy.browser.children")
	})
	b.block("catalog_surface_tokens", func(c *chainBuilder) {
		c.text("Surface tokens: open clear rect frame round_rect round_frame text text_utf8 clip translate char poll pos size event_clear", 24, 24, 148, 163, 184)
	})
	b.block("catalog_record_tokens", func(c *chainBuilder) {
		c.text("Record tokens: pack pack_empty pack_hash at token_at payload_at insert replace delete count valid", 24, 24, 148, 163, 184)
	})
	b.block("catalog_graph_tokens", func(c *chainBuilder) {
		c.text("Graph tokens: graph_children graph_child_at open_child view_push view_pop publish_view link", 24, 24, 148, 163, 184)
	})
	b.block("catalog_payload_tokens", func(c *chainBuilder) {
		c.text("Payload tokens: payload_u64_le payload_hash32 payload_bytes bytes_empty hash_hex u64_dec_bytes utf8_from_codepoint utf8_drop_last", 24, 24, 148, 163, 184)
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
		c.add("call", refreshBootBrowser[:])
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
	}

	setDirty := func(c *chainBuilder) {
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
	}

	markDirty := b.block("mark_dirty", func(c *chainBuilder) {
		setDirty(c)
	})

	initVars := b.block("init_boot_editor_vars", func(c *chainBuilder) {
		c.add("state_hash_get", nil)
		c.varWrite("boot.editor.view")
		c.pushHash(rootHash)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
		c.add("call", refreshBootBrowser[:])
		c.pushHash(initialStageData)
		c.add("dup", nil)
		c.varWrite("toy.stage.data")
		c.varWrite("toy.stage.prev")
		c.pushU64(invalidIndex)
		c.varWrite("toy.stage.selected_index")
		c.pushU64(0)
		c.varWrite("toy.stage.dragging")
		c.pushU64(0)
		c.varWrite("toy.stage.drag_dx")
		c.pushU64(0)
		c.varWrite("toy.stage.drag_dy")
		c.pushHash(toyTitleBytes)
		c.varWrite("toy.title.bytes")
		c.pushHash(toyGalleryRoot)
		c.add("dup", nil)
		c.varWrite("toy.browser.view")
		c.varWrite("toy.browser.selected")
		c.add("call", refreshToyBrowser[:])
		c.pushU64(0)
		c.varWrite("toy.mode")
		c.pushHash(toyMessageReady)
		c.varWrite("toy.message")
		c.pushHash(emptyHash)
		c.varWrite("toy.last.runner")
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
		c.add("call", refreshBootBrowser[:])
		setDirty(c)
	})

	browserCatalog := b.block("browser_catalog", func(c *chainBuilder) {
		c.pushHash(catalogRoot)
		c.add("dup", nil)
		c.varWrite("boot.browser.view")
		c.varWrite("boot.browser.token")
<<<<<<< HEAD
		c.add("call", refreshBootBrowser[:])
		setDirty(c)
=======
		c.pushHash(t.must("noop"))
		c.varWrite("boot.browser.token")
		c.pushU64(0)
		c.varWrite("boot.editor.index")
<<<<<<< HEAD
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
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
		c.add("call", refreshBootBrowser[:])
		setDirty(c)
	})

	publishSelectedToken := b.block("publish_selected_token", func(c *chainBuilder) {
		c.add("call", wrapSelectedTokenStore[:])
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.add("call", refreshBootBrowser[:])
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
		c.add("call", refreshBootBrowser[:])
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
		c.add("call", refreshBootBrowser[:])
		setDirty(c)
	})

	selectors := make([][hashSize]byte, 8)
	for i := range selectors {
		idx := uint64(i)
		selectors[i] = b.block(fmt.Sprintf("select_row_%d", i), func(c *chainBuilder) {
			c.pushU64(idx)
			c.varWrite("boot.editor.index")
			setDirty(c)
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
			c.add("call", refreshBootBrowser[:])
			setDirty(c)
		})
	}

	toySetHome := b.block("toy_set_mode_home", func(c *chainBuilder) {
		c.pushU64(0)
		c.varWrite("toy.mode")
		c.pushHash(toyMessageReady)
		c.varWrite("toy.message")
		setDirty(c)
	})

	toySetInspector := b.block("toy_set_mode_inspector", func(c *chainBuilder) {
		c.pushU64(1)
		c.varWrite("toy.mode")
		setDirty(c)
	})

	toySetTextEdit := b.block("toy_set_mode_text_edit", func(c *chainBuilder) {
		c.pushU64(2)
		c.varWrite("toy.mode")
		c.pushHash(toyMessageEditText)
		c.varWrite("toy.message")
		setDirty(c)
	})

	toySetRoot := b.block("toy_browser_root", func(c *chainBuilder) {
		c.pushHash(rootHash)
		c.add("dup", nil)
		c.varWrite("toy.browser.view")
		c.varWrite("toy.browser.selected")
		c.add("call", refreshToyBrowser[:])
		setDirty(c)
	})

	toySetGallery := b.block("toy_browser_gallery", func(c *chainBuilder) {
		c.pushHash(toyGalleryRoot)
		c.add("dup", nil)
		c.varWrite("toy.browser.view")
		c.varWrite("toy.browser.selected")
		c.add("call", refreshToyBrowser[:])
		setDirty(c)
	})

	toySetCatalog := b.block("toy_browser_catalog", func(c *chainBuilder) {
		c.pushHash(catalogRoot)
		c.add("dup", nil)
		c.varWrite("toy.browser.view")
		c.varWrite("toy.browser.selected")
		c.add("call", refreshToyBrowser[:])
		setDirty(c)
	})

	toySavePrev := b.block("toy_save_prev", func(c *chainBuilder) {
		c.varRead("toy.stage.data")
		c.varWrite("toy.stage.prev")
	})

	toyClearSelection := b.block("toy_clear_selection", func(c *chainBuilder) {
		c.pushU64(invalidIndex)
		c.varWrite("toy.stage.selected_index")
		c.pushU64(0)
		c.varWrite("toy.stage.dragging")
	})

	appendStaticObject := func(name string, rec [hashSize]byte) [hashSize]byte {
		return b.block(name, func(c *chainBuilder) {
			c.add("call", toySavePrev[:])
			c.varRead("toy.stage.data")
			c.add("records_count", nil)
			c.varWrite("toy.stage.selected_index")
			c.varRead("toy.stage.data")
			c.varRead("toy.stage.selected_index")
			c.pushHash(rec)
			c.add("records_insert", nil)
			c.varWrite("toy.stage.data")
			c.pushU64(0)
			c.varWrite("toy.stage.dragging")
			setDirty(c)
		})
	}

	toyAddShape := appendStaticObject("toy_add_shape", shapeRecord)
	toyAddRound := appendStaticObject("toy_add_round", roundRecord)
	toyAddFrame := appendStaticObject("toy_add_frame", frameRecord)
	toyAddSticker := appendStaticObject("toy_add_sticker", stickerRecord)
	toyAddBadge := appendStaticObject("toy_add_badge", badgeRecord)
	toyAddSparkle := appendStaticObject("toy_add_random", sparkleRecord)
	toyAddAnimated := appendStaticObject("toy_add_animated", animatedRecord)

	toyAddExternal := b.block("toy_add_external", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.varRead("toy.stage.data")
		c.add("records_count", nil)
		c.varWrite("toy.stage.selected_index")
		c.add("payload_bytes", make([]byte, 128))
		c.varWrite("toy.new.payload")
		writeField := func(off, val uint64) {
			c.varRead("toy.new.payload")
			c.pushU64(off)
			c.pushU64(val)
			c.add("u64_le_put", nil)
			c.varWrite("toy.new.payload")
		}
		writeField(0, 6)
		writeField(8, 260)
		writeField(16, 140)
		writeField(24, 280)
		writeField(32, 180)
		writeField(40, toyColor(30, 41, 59))
		writeField(48, 18)
		c.varRead("toy.new.payload")
		c.pushU64(64)
		c.pushU64(32)
		c.varRead("toy.browser.selected")
		c.add("hash_to_bytes", nil)
		c.add("bytes_replace", nil)
		c.varWrite("toy.new.payload")
		c.varRead("toy.stage.data")
		c.varRead("toy.stage.selected_index")
		c.pushHash(t.must("noop"))
		c.varRead("toy.new.payload")
		c.add("record_pack", nil)
		c.add("records_insert", nil)
		c.varWrite("toy.stage.data")
		setDirty(c)
	})

	toyParseObject := b.block("toy_parse_object", func(c *chainBuilder) {
		fields := []struct {
			name string
			off  uint64
		}{
			{"toy.obj.type", 0}, {"toy.obj.x", 8}, {"toy.obj.y", 16}, {"toy.obj.w", 24},
			{"toy.obj.h", 32}, {"toy.obj.color", 40}, {"toy.obj.radius", 48}, {"toy.obj.flags", 56},
		}
		for _, f := range fields {
			c.varRead("toy.obj.payload")
			c.pushU64(f.off)
			c.add("u64_le_read", nil)
			c.varWrite(f.name)
		}
		c.varRead("toy.obj.payload")
		c.pushU64(64)
		c.add("bytes_drop", nil)
		c.pushU64(32)
		c.add("bytes_take", nil)
		c.add("bytes_to_hash32", nil)
		c.varWrite("toy.obj.ref")
	})

	toyDrawSelectedFrame := b.block("toy_draw_selected_frame", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.pushColor(255, 255, 255)
		c.add("surface_frame", nil)
		c.varRead("toy.obj.x")
		c.pushU64(2)
		c.add("sub", nil)
		c.varRead("toy.obj.y")
		c.pushU64(2)
		c.add("sub", nil)
		c.varRead("toy.obj.w")
		c.pushU64(4)
		c.add("add", nil)
		c.varRead("toy.obj.h")
		c.pushU64(4)
		c.add("add", nil)
		c.add("rect_make", nil)
		c.pushColor(14, 165, 233)
		c.add("surface_frame", nil)
	})

	toyRenderRect := b.block("toy_render_rect_object", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.varRead("toy.obj.color")
		c.add("surface_rect", nil)
	})

	toyRenderRound := b.block("toy_render_round_object", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.varRead("toy.obj.color")
		c.varRead("toy.obj.radius")
		c.add("surface_round_rect", nil)
	})

	toyRenderFrame := b.block("toy_render_frame_object", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.varRead("toy.obj.color")
		c.add("surface_frame", nil)
	})

	toyRenderText := b.block("toy_render_text_object", func(c *chainBuilder) {
		c.varRead("toy.obj.ref")
		c.varRead("toy.obj.x")
		c.varRead("toy.obj.y")
		c.varRead("toy.obj.color")
		c.add("surface_text_utf8", nil)
	})

	toyRenderBadge := b.block("toy_render_badge_object", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.varRead("toy.obj.color")
		c.varRead("toy.obj.radius")
		c.add("surface_round_rect", nil)
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.pushColor(255, 255, 255)
		c.varRead("toy.obj.radius")
		c.add("surface_round_frame", nil)
		c.varRead("toy.obj.ref")
		c.varRead("toy.obj.x")
		c.pushU64(18)
		c.add("add", nil)
		c.varRead("toy.obj.y")
		c.pushU64(28)
		c.add("add", nil)
		c.pushColor(30, 41, 59)
		c.add("surface_text_utf8", nil)
	})

	toyRenderExternal := b.block("toy_render_external_object", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.pushColor(15, 23, 42)
		c.pushU64(18)
		c.add("surface_round_rect", nil)
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.add("surface_clip_push", nil)
		c.varRead("toy.obj.x")
		c.varRead("toy.obj.y")
		c.add("surface_translate_push", nil)
		c.varRead("toy.obj.ref")
		c.add("call_stack", nil)
		c.add("pop", nil)
		c.add("surface_translate_pop", nil)
		c.add("surface_clip_pop", nil)
	})

	toyRenderAnimated := b.block("toy_render_animated_object", func(c *chainBuilder) {
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.pushU64(80)
		c.pushU64(120)
		c.add("time_ms", nil)
		c.pushU64(175)
		c.add("u64_mod", nil)
		c.pushU64(80)
		c.add("add", nil)
		c.add("color_rgb", nil)
		c.varRead("toy.obj.radius")
		c.add("surface_round_rect", nil)
		c.varRead("toy.obj.ref")
		c.varRead("toy.obj.x")
		c.pushU64(22)
		c.add("add", nil)
		c.varRead("toy.obj.y")
		c.pushU64(28)
		c.add("add", nil)
		c.pushColor(255, 255, 255)
		c.add("surface_text_utf8", nil)
	})

	toyRenderObject := b.block("toy_render_object", func(c *chainBuilder) {
		c.varRead("toy.obj.type")
		c.pushU64(1)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderRect[:])
		c.varRead("toy.obj.type")
		c.pushU64(2)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderRound[:])
		c.varRead("toy.obj.type")
		c.pushU64(3)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderFrame[:])
		c.varRead("toy.obj.type")
		c.pushU64(4)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderText[:])
		c.varRead("toy.obj.type")
		c.pushU64(5)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderBadge[:])
		c.varRead("toy.obj.type")
		c.pushU64(6)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderExternal[:])
		c.varRead("toy.obj.type")
		c.pushU64(7)
		c.add("eq", nil)
		c.add("call_cond_static", toyRenderAnimated[:])
		c.varRead("toy.render.index")
		c.varRead("toy.stage.selected_index")
		c.add("eq", nil)
		c.varRead("toy.render.selection")
		c.add("and", nil)
		c.add("call_cond_static", toyDrawSelectedFrame[:])
	})

	toyRenderOneAndInc := b.block("toy_render_one_and_inc", func(c *chainBuilder) {
		c.varRead("toy.render.data")
		c.varRead("toy.render.index")
		c.add("records_at", nil)
		c.pushU64(0)
		c.add("record_payload_at", nil)
		c.varWrite("toy.obj.payload")
		c.add("call", toyParseObject[:])
		c.add("call", toyRenderObject[:])
		c.varRead("toy.render.index")
		c.add("u64_inc", nil)
		c.varWrite("toy.render.index")
	})

	toyRenderStageLoopBody := b.block("toy_render_stage_loop_body", func(c *chainBuilder) {
		c.varRead("toy.render.index")
		c.varRead("toy.render.data")
		c.add("records_count", nil)
		c.add("lt", nil)
		c.add("call_cond_static", toyRenderOneAndInc[:])
		c.varRead("toy.render.index")
		c.varRead("toy.render.data")
		c.add("records_count", nil)
		c.add("lt", nil)
		c.add("again_cond", nil)
	})

	toyRenderStageLoop := b.block("toy_render_stage_loop", func(c *chainBuilder) {
		c.pushU64(0)
		c.varWrite("toy.render.index")
		c.add("call", toyRenderStageLoopBody[:])
	})

	toyDrawStage := b.block("toy_draw_stage", func(c *chainBuilder) {
		c.rect(320, 120, 650, 450)
		c.pushColor(10, 15, 25)
		c.pushU64(26)
		c.add("surface_round_rect", nil)
		c.rect(320, 120, 650, 450)
		c.pushColor(71, 85, 105)
		c.pushU64(26)
		c.add("surface_round_frame", nil)
		c.rect(320, 120, 650, 450)
		c.add("surface_clip_push", nil)
		c.pushU64(320)
		c.pushU64(120)
		c.add("surface_translate_push", nil)
		c.pushU64(1)
		c.varWrite("toy.render.selection")
		c.varRead("toy.stage.data")
		c.varWrite("toy.render.data")
		c.add("call", toyRenderStageLoop[:])
		c.add("surface_translate_pop", nil)
		c.add("surface_clip_pop", nil)
	})

	toyRuntimeDrawStage := b.block("toy_runtime_draw_stage", func(c *chainBuilder) {
		c.pushColor(15, 23, 42)
		c.add("surface_clear", nil)
		c.pushU64(0)
		c.varWrite("toy.render.selection")
		c.varRead("toy.runtime.stage.data")
		c.varWrite("toy.render.data")
		c.add("call", toyRenderStageLoop[:])
	})

	toyRuntimeFrameLoop := b.block("toy_runtime_frame_loop", func(c *chainBuilder) {
		c.add("call", toyRuntimeDrawStage[:])
		c.add("surface_poll", nil)
		c.varWrite("toy.runtime.event")
		c.varRead("toy.runtime.event")
		c.pushU64(0xffffffff)
		c.add("ne", nil)
		c.add("surface_event_clear", nil)
		c.pushU64(33)
		c.add("sleep_ms", nil)
		c.add("again_cond", nil)
	})

	toyRuntimeStandalone := b.block("toy_runtime_standalone", func(c *chainBuilder) {
		c.pushU64(960)
		c.pushU64(640)
		c.add("surface_open", nil)
		c.add("pop", nil)
		c.add("surface_event_clear", nil)
		c.add("call", toyRuntimeFrameLoop[:])
		c.add("surface_close", nil)
	})

	toyRuntimeRenderStage := b.block("toy_runtime_render_stage", func(c *chainBuilder) {
		c.add("surface_is_open", nil)
		c.varWrite("toy.runtime.surface_open")
		c.varRead("toy.runtime.surface_open")
		c.add("call_cond_static", toyRuntimeDrawStage[:])
		c.varRead("toy.runtime.surface_open")
		c.add("not", nil)
		c.add("call_cond_static", toyRuntimeStandalone[:])
	})

	toySelectedValidCore := func(c *chainBuilder) {
		c.varRead("toy.stage.selected_index")
		c.varRead("toy.stage.data")
		c.add("records_count", nil)
		c.add("lt", nil)
	}

	toyLoadSelectedPayload := b.block("toy_load_selected_payload", func(c *chainBuilder) {
		c.varRead("toy.stage.data")
		c.varRead("toy.stage.selected_index")
		c.add("records_at", nil)
		c.pushU64(0)
		c.add("record_payload_at", nil)
		c.varWrite("toy.mutate.payload")
	})

	toyStoreSelectedPayload := b.block("toy_store_selected_payload", func(c *chainBuilder) {
		c.varRead("toy.stage.data")
		c.varRead("toy.stage.selected_index")
		c.pushHash(t.must("noop"))
		c.varRead("toy.mutate.payload")
		c.add("record_pack", nil)
		c.add("records_replace", nil)
		c.varWrite("toy.stage.data")
	})

	toyMutateColorCore := b.block("toy_mutate_color_core", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.add("call", toyLoadSelectedPayload[:])
		c.varRead("toy.mutate.payload")
		c.pushU64(40)
		c.add("random_u64", nil)
		c.pushU64(255)
		c.add("u64_mod", nil)
		c.add("random_u64", nil)
		c.pushU64(200)
		c.add("u64_mod", nil)
		c.pushU64(40)
		c.add("add", nil)
		c.add("random_u64", nil)
		c.pushU64(180)
		c.add("u64_mod", nil)
		c.pushU64(60)
		c.add("add", nil)
		c.add("color_rgb", nil)
		c.add("u64_le_put", nil)
		c.varWrite("toy.mutate.payload")
		c.add("call", toyStoreSelectedPayload[:])
		setDirty(c)
	})

	toyMutateColor := b.block("toy_mutate_color", func(c *chainBuilder) {
		toySelectedValidCore(c)
		c.add("call_cond_static", toyMutateColorCore[:])
	})

	writeScaledField := func(c *chainBuilder, off, mulBy, divBy, minVal uint64) {
		c.varRead("toy.mutate.payload")
		c.pushU64(off)
		c.varRead("toy.mutate.payload")
		c.pushU64(off)
		c.add("u64_le_read", nil)
		c.pushU64(mulBy)
		c.add("mul", nil)
		c.pushU64(divBy)
		c.add("div", nil)
		c.pushU64(minVal)
		c.add("u64_max", nil)
		c.add("u64_le_put", nil)
		c.varWrite("toy.mutate.payload")
	}

	toyGrowCore := b.block("toy_grow_core", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.add("call", toyLoadSelectedPayload[:])
		writeScaledField(c, 24, 6, 5, 30)
		writeScaledField(c, 32, 6, 5, 30)
		c.add("call", toyStoreSelectedPayload[:])
		setDirty(c)
	})

	toyShrinkCore := b.block("toy_shrink_core", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.add("call", toyLoadSelectedPayload[:])
		writeScaledField(c, 24, 4, 5, 30)
		writeScaledField(c, 32, 4, 5, 30)
		c.add("call", toyStoreSelectedPayload[:])
		setDirty(c)
	})

	toyGrow := b.block("toy_mutate_size_grow", func(c *chainBuilder) {
		toySelectedValidCore(c)
		c.add("call_cond_static", toyGrowCore[:])
	})

	toyShrink := b.block("toy_mutate_size_shrink", func(c *chainBuilder) {
		toySelectedValidCore(c)
		c.add("call_cond_static", toyShrinkCore[:])
	})

	toyDuplicateCore := b.block("toy_duplicate_selected_core", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.varRead("toy.stage.data")
		c.varRead("toy.stage.selected_index")
		c.add("records_at", nil)
		c.varWrite("toy.copy.record")
		c.varRead("toy.stage.data")
		c.add("records_count", nil)
		c.varWrite("toy.stage.selected_index")
		c.varRead("toy.stage.data")
		c.varRead("toy.stage.selected_index")
		c.varRead("toy.copy.record")
		c.add("records_insert", nil)
		c.varWrite("toy.stage.data")
		setDirty(c)
	})

	toyDuplicate := b.block("toy_duplicate_selected", func(c *chainBuilder) {
		toySelectedValidCore(c)
		c.add("call_cond_static", toyDuplicateCore[:])
	})

	toyUndo := b.block("toy_undo", func(c *chainBuilder) {
		c.varRead("toy.stage.prev")
		c.varWrite("toy.stage.data")
		c.add("call", toyClearSelection[:])
		setDirty(c)
	})

	toyClear := b.block("toy_clear", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.add("records_empty", nil)
		c.varWrite("toy.stage.data")
		c.add("call", toyClearSelection[:])
		setDirty(c)
	})

	toySetSelectedRefToTitleCore := b.block("toy_set_selected_ref_to_title_core", func(c *chainBuilder) {
		c.add("call", toyLoadSelectedPayload[:])
		c.varRead("toy.mutate.payload")
		c.pushU64(64)
		c.pushU64(32)
		c.varRead("toy.title.bytes")
		c.add("hash_to_bytes", nil)
		c.add("bytes_replace", nil)
		c.varWrite("toy.mutate.payload")
		c.add("call", toyStoreSelectedPayload[:])
	})

	toySetSelectedRefToTitle := b.block("toy_set_selected_ref_to_title", func(c *chainBuilder) {
		toySelectedValidCore(c)
		c.add("call_cond_static", toySetSelectedRefToTitleCore[:])
	})

	toyTextDropLast := b.block("toy_text_drop_last", func(c *chainBuilder) {
		c.varRead("toy.title.bytes")
		c.add("utf8_drop_last", nil)
		c.varWrite("toy.title.bytes")
		c.add("call", toySetSelectedRefToTitle[:])
		setDirty(c)
	})

	toyTextAppendChar := b.block("toy_text_append_char", func(c *chainBuilder) {
		c.varRead("toy.title.bytes")
		c.varRead("toy.char")
		c.add("utf8_from_codepoint", nil)
		c.add("concat", nil)
		c.varWrite("toy.title.bytes")
		c.add("call", toySetSelectedRefToTitle[:])
		setDirty(c)
	})

	toyTextInputCore := b.block("toy_text_input_core", func(c *chainBuilder) {
		c.add("surface_char", nil)
		c.varWrite("toy.char")
		c.varRead("toy.char")
		c.pushU64(8)
		c.add("eq", nil)
		c.add("call_cond_static", toyTextDropLast[:])
		c.varRead("toy.char")
		c.pushU64(0)
		c.add("ne", nil)
		c.varRead("toy.char")
		c.pushU64(8)
		c.add("ne", nil)
		c.add("and", nil)
		c.add("call_cond_static", toyTextAppendChar[:])
	})

	toyTextInputDispatch := b.block("toy_text_input_dispatch", func(c *chainBuilder) {
		c.varRead("toy.mode")
		c.pushU64(2)
		c.add("eq", nil)
		c.add("call_cond_static", toyTextInputCore[:])
	})

	toyHitSelectCurrent := b.block("toy_hit_select_current", func(c *chainBuilder) {
		c.varRead("toy.render.index")
		c.varWrite("toy.stage.selected_index")
		c.pushU64(1)
		c.varWrite("toy.stage.dragging")
		c.add("surface_pos", nil)
		c.add("pair_first", nil)
		c.varRead("toy.obj.x")
		c.add("sub", nil)
		c.varWrite("toy.stage.drag_dx")
		c.add("surface_pos", nil)
		c.add("pair_second", nil)
		c.varRead("toy.obj.y")
		c.add("sub", nil)
		c.varWrite("toy.stage.drag_dy")
	})

	toyHitCurrent := b.block("toy_hit_current_object", func(c *chainBuilder) {
		c.varRead("toy.render.data")
		c.varRead("toy.render.index")
		c.add("records_at", nil)
		c.pushU64(0)
		c.add("record_payload_at", nil)
		c.varWrite("toy.obj.payload")
		c.add("call", toyParseObject[:])
		c.rectVars("toy.obj.x", "toy.obj.y", "toy.obj.w", "toy.obj.h")
		c.add("surface_pos", nil)
		c.add("pair_first", nil)
		c.add("surface_pos", nil)
		c.add("pair_second", nil)
		c.add("rect_contains", nil)
		c.add("call_cond_static", toyHitSelectCurrent[:])
		c.varRead("toy.render.index")
		c.add("u64_inc", nil)
		c.varWrite("toy.render.index")
	})

	toyHitLoopBody := b.block("toy_hit_test_loop_body", func(c *chainBuilder) {
		c.varRead("toy.render.index")
		c.varRead("toy.render.data")
		c.add("records_count", nil)
		c.add("lt", nil)
		c.add("call_cond_static", toyHitCurrent[:])
		c.varRead("toy.render.index")
		c.varRead("toy.render.data")
		c.add("records_count", nil)
		c.add("lt", nil)
		c.add("again_cond", nil)
	})

	toyHitTestLoop := b.block("toy_hit_test_loop", func(c *chainBuilder) {
		c.add("call", toySavePrev[:])
		c.add("call", toyClearSelection[:])
		c.varRead("toy.stage.data")
		c.varWrite("toy.render.data")
		c.pushU64(0)
		c.varWrite("toy.render.index")
		c.add("call", toyHitLoopBody[:])
		setDirty(c)
	})

	toyDragUpdateCore := b.block("toy_drag_update_core", func(c *chainBuilder) {
		c.add("call", toyLoadSelectedPayload[:])
		c.varRead("toy.mutate.payload")
		c.pushU64(8)
		c.add("surface_pos", nil)
		c.add("pair_first", nil)
		c.varRead("toy.stage.drag_dx")
		c.add("sub", nil)
		c.add("u64_le_put", nil)
		c.varWrite("toy.mutate.payload")
		c.varRead("toy.mutate.payload")
		c.pushU64(16)
		c.add("surface_pos", nil)
		c.add("pair_second", nil)
		c.varRead("toy.stage.drag_dy")
		c.add("sub", nil)
		c.add("u64_le_put", nil)
		c.varWrite("toy.mutate.payload")
		c.add("call", toyStoreSelectedPayload[:])
		setDirty(c)
	})

	toyDragUpdate := b.block("toy_drag_update", func(c *chainBuilder) {
		c.varRead("toy.stage.dragging")
		c.add("call_cond_static", toyDragUpdateCore[:])
	})

	toyStopDrag := b.block("toy_stop_drag", func(c *chainBuilder) {
		c.pushU64(0)
		c.varWrite("toy.stage.dragging")
	})

	toyPlaySelectedInline := b.block("toy_play_selected_inline", func(c *chainBuilder) {
		c.varRead("toy.browser.selected")
		c.add("call_stack", nil)
		c.add("pop", nil)
		setDirty(c)
	})

	toyShelfEmpty := b.block("toy_shelf_empty", func(c *chainBuilder) {
		c.add("records_empty", nil)
		c.varWrite("toy.shelf.data")
	})

	toyAppendShelf := b.block("toy_append_shelf", func(c *chainBuilder) {
		c.pushHash(toyShelfKey)
		c.add("uget", nil)
		c.varWrite("toy.shelf.data")
		c.varRead("toy.shelf.data")
		c.add("records_valid", nil)
		c.add("not", nil)
		c.add("call_cond_static", toyShelfEmpty[:])
		c.varRead("toy.shelf.data")
		c.add("dup", nil)
		c.add("records_count", nil)
		c.pushHash(t.must("noop"))
		c.varRead("toy.last.runner")
		c.add("record_pack_hash", nil)
		c.add("records_insert", nil)
		c.varWrite("toy.shelf.data")
		c.pushHash(toyShelfKey)
		c.varRead("toy.shelf.data")
		c.add("uset", nil)
		c.add("pop", nil)
	})

	appendRunnerRecord := func(c *chainBuilder, emit func(*chainBuilder)) {
		c.varRead("toy.publish.runner")
		c.add("dup", nil)
		c.add("records_count", nil)
		emit(c)
		c.add("records_insert", nil)
		c.varWrite("toy.publish.runner")
	}

	toyBuildRunner := b.block("toy_build_runner", func(c *chainBuilder) {
		c.add("records_empty", nil)
		c.varWrite("toy.publish.runner")
		appendRunnerRecord(c, func(c *chainBuilder) {
			c.pushHash(t.must("noop"))
			c.add("payload_bytes", []byte("CVM_TOY_STAGE_V1"))
			c.add("record_pack", nil)
		})
		appendRunnerRecord(c, func(c *chainBuilder) {
			c.pushHash(t.must("payload_hash32"))
			c.varRead("toy.stage.data")
			c.add("record_pack_hash", nil)
		})
		appendRunnerRecord(c, func(c *chainBuilder) {
			c.pushHash(t.must("var_write"))
			c.add("payload_bytes", []byte("toy.runtime.stage.data"))
			c.add("record_pack", nil)
		})
		appendRunnerRecord(c, func(c *chainBuilder) {
			c.pushHash(t.must("payload_hash32"))
			c.varRead("toy.title.bytes")
			c.add("record_pack_hash", nil)
		})
		appendRunnerRecord(c, func(c *chainBuilder) {
			c.pushHash(t.must("var_write"))
			c.add("payload_bytes", []byte("toy.runtime.title.bytes"))
			c.add("record_pack", nil)
		})
		appendRunnerRecord(c, func(c *chainBuilder) {
			c.pushHash(t.must("call"))
			c.pushHash(toyRuntimeRenderStage)
			c.add("record_pack_hash", nil)
		})
		c.varRead("toy.publish.runner")
		c.varWrite("toy.last.runner")
		c.varRead("toy.last.runner")
		c.add("upload", nil)
		c.add("pop", nil)
	})

	toyPublish := b.block("toy_publish", func(c *chainBuilder) {
		c.add("call", toyBuildRunner[:])
		c.varRead("toy.last.runner")
		c.add("state_hash_set", nil)
		c.add("publish_view", nil)
		c.pushHash(toyGalleryRoot)
		c.varRead("toy.last.runner")
		c.add("graph_link", nil)
		c.add("pop", nil)
		c.pushHash(toyGalleryRoot)
		c.varRead("toy.last.runner")
		c.add("vote", nil)
		c.add("pop", nil)
		c.add("call", toyAppendShelf[:])
		c.pushHash(toyMessagePublished)
		c.varWrite("toy.message")
		c.add("call", refreshToyBrowser[:])
		setDirty(c)
	})

	toySetBoot := b.block("toy_set_boot", func(c *chainBuilder) {
		c.add("call", toyBuildRunner[:])
		c.varRead("toy.last.runner")
		c.add("state_hash_set", nil)
		c.add("save_boot", nil)
		c.pushHash(toyMessageBoot)
		c.varWrite("toy.message")
		setDirty(c)
	})

	toyPublishSetBoot := b.block("toy_publish_set_boot", func(c *chainBuilder) {
		c.add("call", toyPublish[:])
		c.varRead("toy.last.runner")
		c.add("state_hash_set", nil)
		c.add("save_boot", nil)
		c.pushHash(toyMessageBoot)
		c.varWrite("toy.message")
		setDirty(c)
	})

	toyBrowserSelectors := make([][hashSize]byte, 6)
	for i := range toyBrowserSelectors {
		idx := uint64(i)
		toyBrowserSelectors[i] = b.block(fmt.Sprintf("toy_browse_child_%d", i), func(c *chainBuilder) {
			c.varRead("toy.browser.view")
			c.add("state_hash_set", nil)
			c.pushU64(idx)
			c.add("state_index_set", nil)
			c.add("open_child", nil)
			c.add("state_hash_get", nil)
			c.add("dup", nil)
			c.varWrite("toy.browser.view")
			c.varWrite("toy.browser.selected")
			c.add("call", refreshToyBrowser[:])
			setDirty(c)
		})
	}

	toyMouseDownStage := b.block("toy_mouse_down_stage", func(c *chainBuilder) {
		c.pushU64(320)
		c.pushU64(120)
		c.add("surface_translate_push", nil)
		c.add("call", toyHitTestLoop[:])
		c.add("surface_translate_pop", nil)
	})

	toyMouseMoveHome := b.block("toy_mouse_move_home", func(c *chainBuilder) {
		c.pushU64(320)
		c.pushU64(120)
		c.add("surface_translate_push", nil)
		c.add("call", toyDragUpdate[:])
		c.add("surface_translate_pop", nil)
	})

	toyMouseMove := b.block("toy_mouse_move", func(c *chainBuilder) {
		c.varRead("toy.mode")
		c.pushU64(1)
		c.add("ne", nil)
		c.add("call_cond_static", toyMouseMoveHome[:])
	})

	toyHomeMouse := b.block("toy_dispatch_mouse", func(c *chainBuilder) {
		c.rectContains(320, 120, 650, 450)
		c.add("call_cond_static", toyMouseDownStage[:])
		c.rectContains(28, 132, 250, 42)
		c.add("call_cond_static", toyAddShape[:])
		c.rectContains(28, 184, 250, 42)
		c.add("call_cond_static", toyAddRound[:])
		c.rectContains(28, 236, 250, 42)
		c.add("call_cond_static", toyAddFrame[:])
		c.rectContains(28, 288, 250, 42)
		c.add("call_cond_static", toyAddSticker[:])
		c.rectContains(28, 340, 250, 42)
		c.add("call_cond_static", toyAddBadge[:])
		c.rectContains(28, 392, 250, 42)
		c.add("call_cond_static", toyAddSparkle[:])
		c.rectContains(28, 444, 250, 42)
		c.add("call_cond_static", toyAddAnimated[:])
		c.rectContains(28, 496, 250, 42)
		c.add("call_cond_static", toyAddExternal[:])
		for i, target := range toyBrowserSelectors {
			c.rectContains(1004, uint64(166+i*52), 248, 42)
			c.add("call_cond_static", target[:])
		}
		c.rectContains(998, 542, 82, 34)
		c.add("call_cond_static", toyPlaySelectedInline[:])
		c.rectContains(1088, 542, 84, 34)
		c.add("call_cond_static", toyAddExternal[:])
		c.rectContains(1180, 542, 72, 34)
		c.add("call_cond_static", toySetRoot[:])
		c.rectContains(998, 586, 82, 34)
		c.add("call_cond_static", toySetGallery[:])
		c.rectContains(1088, 586, 84, 34)
		c.add("call_cond_static", toySetCatalog[:])
		c.rectContains(320, 590, 82, 40)
		c.add("call_cond_static", toyMutateColor[:])
		c.rectContains(410, 590, 72, 40)
		c.add("call_cond_static", toyGrow[:])
		c.rectContains(490, 590, 72, 40)
		c.add("call_cond_static", toyShrink[:])
		c.rectContains(570, 590, 72, 40)
		c.add("call_cond_static", toyDuplicate[:])
		c.rectContains(650, 590, 72, 40)
		c.add("call_cond_static", toyUndo[:])
		c.rectContains(730, 590, 72, 40)
		c.add("call_cond_static", toyClear[:])
		c.rectContains(810, 590, 92, 40)
		c.add("call_cond_static", toySetTextEdit[:])
		c.rectContains(910, 590, 60, 40)
		c.add("call_cond_static", toySetHome[:])
		c.rectContains(320, 646, 96, 40)
		c.add("call_cond_static", toyPublish[:])
		c.rectContains(428, 646, 104, 40)
		c.add("call_cond_static", toySetBoot[:])
		c.rectContains(544, 646, 148, 40)
		c.add("call_cond_static", toyPublishSetBoot[:])
		c.rectContains(704, 646, 100, 40)
		c.add("call_cond_static", toySetInspector[:])
		c.rectContains(816, 646, 120, 40)
		c.add("call_cond_static", toySetCatalog[:])
	})

	inspectorMouse := b.block("inspector_mouse_dispatch", func(c *chainBuilder) {
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
		c.rectContains(1060, 588, 160, 40)
		c.add("call_cond_static", toySetHome[:])
	})

	mouse := b.block("mouse_dispatch", func(c *chainBuilder) {
		c.varRead("toy.mode")
		c.pushU64(1)
		c.add("eq", nil)
		c.add("call_cond_static", inspectorMouse[:])
		c.varRead("toy.mode")
		c.pushU64(1)
		c.add("ne", nil)
		c.add("call_cond_static", toyHomeMouse[:])
	})

	inspectorDraw := b.block("boot_editor_draw", func(c *chainBuilder) {
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
			c.varRead("boot.browser.children")
			c.pushU64(uint64(i))
			c.add("child_at", nil)
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
		c.buttonUTF8("返回玩具", 1060, 588, 160, 40, 67, 120, 86)
		c.text("Catalog rows: 00 rounded, 01 badge, 02 shapes, 03 surface, 04 records, 05 graph, 06 payload, 07 state.", 660, 650, 148, 163, 184)
	})

<<<<<<< HEAD
	toyDrawTextEditOverlay := b.block("toy_draw_text_edit_overlay", func(c *chainBuilder) {
		c.rect(320, 34, 650, 62)
		c.pushColor(76, 29, 149)
		c.pushU64(20)
		c.add("surface_round_rect", nil)
		c.textUTF8("文字编辑中", 342, 50, 255, 255, 255)
		c.varRead("toy.title.bytes")
		c.pushU64(456)
		c.pushU64(50)
		c.pushColor(253, 244, 255)
		c.add("surface_text_utf8", nil)
		c.textUTF8("输入会写入当前选中贴纸/徽章，点“完成”返回。", 342, 74, 221, 214, 254)
	})

	toyDrawHome := b.block("toy_draw_home", func(c *chainBuilder) {
		c.pushColor(11, 18, 32)
		c.add("surface_clear", nil)
		c.rect(0, 0, 1280, 92)
		c.pushColor(17, 24, 39)
		c.add("surface_rect", nil)
		c.textUTF8("第一启动玩具屋", 28, 24, 245, 247, 250)
		c.textUTF8("点一个玩具，摆弄一下，觉得有意思就发布。", 28, 56, 148, 163, 184)
		c.textUTF8("标题：", 330, 26, 125, 211, 252)
		c.varRead("toy.title.bytes")
		c.pushU64(392)
		c.pushU64(26)
		c.pushColor(245, 247, 250)
		c.add("surface_text_utf8", nil)
		c.varRead("toy.message")
		c.pushU64(330)
		c.pushU64(58)
		c.pushColor(196, 181, 253)
		c.add("surface_text_utf8", nil)

		c.rect(18, 108, 276, 466)
		c.pushColor(15, 23, 42)
		c.pushU64(24)
		c.add("surface_round_rect", nil)
		c.textUTF8("玩具箱", 36, 122, 245, 247, 250)
		c.buttonUTF8("色块", 28, 132, 250, 42, 22, 101, 52)
		c.buttonUTF8("软软圆角", 28, 184, 250, 42, 67, 56, 202)
		c.buttonUTF8("边框卡片", 28, 236, 250, 42, 157, 23, 77)
		c.buttonUTF8("文字贴纸", 28, 288, 250, 42, 51, 65, 85)
		c.buttonUTF8("徽章", 28, 340, 250, 42, 180, 83, 9)
		c.buttonUTF8("闪光点", 28, 392, 250, 42, 161, 98, 7)
		c.buttonUTF8("会动的块", 28, 444, 250, 42, 8, 145, 178)
		c.buttonUTF8("加入选中外部块", 28, 496, 250, 42, 30, 64, 175)

		c.textUTF8("舞台", 330, 104, 203, 213, 225)
		c.add("call", toyDrawStage[:])
		c.buttonUTF8("换色", 320, 590, 82, 40, 79, 70, 229)
		c.buttonUTF8("变大", 410, 590, 72, 40, 22, 101, 52)
		c.buttonUTF8("变小", 490, 590, 72, 40, 15, 118, 110)
		c.buttonUTF8("复制", 570, 590, 72, 40, 88, 28, 135)
		c.buttonUTF8("撤销", 650, 590, 72, 40, 113, 63, 18)
		c.buttonUTF8("清空", 730, 590, 72, 40, 127, 29, 29)
		c.buttonUTF8("编辑文字", 810, 590, 92, 40, 91, 33, 182)
		c.buttonUTF8("完成", 910, 590, 60, 40, 22, 78, 99)
		c.buttonUTF8("发布", 320, 646, 96, 40, 22, 101, 52)
		c.buttonUTF8("设为启动", 428, 646, 104, 40, 67, 56, 202)
		c.buttonUTF8("发布并设为启动", 544, 646, 148, 40, 88, 28, 135)
		c.buttonUTF8("Inspector", 704, 646, 100, 40, 55, 65, 81)
		c.buttonUTF8("Token Catalog", 816, 646, 120, 40, 55, 65, 81)

		c.rect(988, 108, 276, 532)
		c.pushColor(15, 23, 42)
		c.pushU64(24)
		c.add("surface_round_rect", nil)
		c.textUTF8("探索", 1006, 124, 245, 247, 250)
		c.textUTF8("玩具流 / 公开根 / 我的架子", 1006, 146, 148, 163, 184)
		for i := 0; i < 6; i++ {
			y := uint64(166 + i*52)
			c.rect(1004, y, 248, 42)
			c.pushColor(30, 41, 59)
			c.pushU64(14)
			c.add("surface_round_rect", nil)
			c.textUTF8(fmt.Sprintf("%02d", i), 1018, y+13, 125, 211, 252)
			c.varRead("toy.browser.children")
			c.pushU64(uint64(i))
			c.add("child_at", nil)
			c.add("hash_hex", nil)
			c.pushU64(1054)
			c.pushU64(y + 13)
			c.pushColor(226, 232, 240)
			c.add("surface_text", nil)
		}
		c.textUTF8("选中：", 1006, 488, 148, 163, 184)
		c.varRead("toy.browser.selected")
		c.add("hash_hex", nil)
		c.pushU64(1058)
		c.pushU64(488)
		c.pushColor(226, 232, 240)
		c.add("surface_text", nil)
		c.buttonUTF8("玩一下", 998, 542, 82, 34, 30, 64, 175)
		c.buttonUTF8("加到舞台", 1088, 542, 84, 34, 22, 101, 52)
		c.buttonUTF8("Root", 1180, 542, 72, 34, 55, 65, 81)
		c.buttonUTF8("玩具流", 998, 586, 82, 34, 79, 70, 229)
		c.buttonUTF8("Catalog", 1088, 586, 84, 34, 55, 65, 81)
		c.varRead("toy.mode")
		c.pushU64(2)
		c.add("eq", nil)
		c.add("call_cond_static", toyDrawTextEditOverlay[:])
	})

	draw := b.block("toy_draw_dispatch", func(c *chainBuilder) {
		c.varRead("toy.mode")
		c.pushU64(1)
		c.add("eq", nil)
		c.add("call_cond_static", inspectorDraw[:])
		c.varRead("toy.mode")
		c.pushU64(1)
		c.add("ne", nil)
		c.add("call_cond_static", toyDrawHome[:])
	})

<<<<<<< HEAD
	drawDirty := b.block("boot_editor_draw_dirty", func(c *chainBuilder) {
		c.add("call", draw[:])
		c.pushU64(0)
		c.varWrite("boot.editor.dirty")
	})

	drawIdleStage := b.block("toy_draw_idle_stage", func(c *chainBuilder) {
		c.varRead("boot.editor.dirty")
		c.add("not", nil)
		c.varRead("toy.mode")
		c.pushU64(1)
		c.add("ne", nil)
		c.add("and", nil)
		c.add("call_cond_static", toyDrawStage[:])
	})

=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
	frameLoop := b.block("boot_editor_frame_loop", func(c *chainBuilder) {
		c.add("call", draw[:])
		c.add("surface_poll", nil)
		c.varWrite("toy.event")
		c.varRead("toy.event")
		c.pushU64(513)
		c.add("eq", nil)
		c.add("call_cond_static", mouse[:])
<<<<<<< HEAD
<<<<<<< HEAD
		c.varRead("toy.event")
		c.pushU64(512)
		c.add("eq", nil)
		c.add("call_cond_static", toyMouseMove[:])
		c.varRead("toy.event")
		c.pushU64(514)
		c.add("eq", nil)
		c.add("call_cond_static", toyStopDrag[:])
		c.varRead("toy.event")
		c.pushU64(258)
		c.add("eq", nil)
		c.add("call_cond_static", toyTextInputDispatch[:])
		c.varRead("toy.event")
		c.pushU64(5)
		c.add("eq", nil)
		c.add("call_cond_static", markDirty[:])
		c.varRead("toy.event")
		c.pushU64(15)
		c.add("eq", nil)
		c.add("call_cond_static", markDirty[:])
		c.add("call", drawIdleStage[:])
		c.varRead("toy.event")
=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
		c.pushU64(0xffffffff)
		c.add("ne", nil)
		c.add("surface_event_clear", nil)
		c.pushU64(33)
		c.add("sleep_ms", nil)
		c.add("again_cond", nil)
	})

	b.block("boot_editor_entry", func(c *chainBuilder) {
		c.add("call", initVars[:])
		c.pushU64(1280)
		c.pushU64(720)
		c.add("surface_open", nil)
		c.add("pop", nil)
<<<<<<< HEAD
<<<<<<< HEAD
		c.add("surface_event_clear", nil)
		c.pushU64(1)
		c.varWrite("boot.editor.dirty")
=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
=======
>>>>>>> parent of 06bb3e2 (Add dirty-flag redraw optimization to boot editor)
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
	toyGalleryRoot := byName["toy_gallery_root"]

	edges := []graphEdge{
		{publicRoot, catalogRoot},
		{publicRoot, toyGalleryRoot},
		{toyGalleryRoot, byName["fragment_round_rect_demo"]},
		{toyGalleryRoot, byName["fragment_text_badge_demo"]},
		{toyGalleryRoot, byName["fragment_color_shape_demo"]},
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
		"surface_round_rect", "surface_round_frame", "surface_text", "surface_text_utf8",
		"surface_clip_push", "surface_clip_pop", "surface_translate_push", "surface_translate_pop",
		"surface_poll", "surface_char", "surface_is_open", "surface_pos", "surface_size", "surface_event_clear", "sleep_ms",
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
		"hash_hex", "u64_dec_bytes", "utf8_from_codepoint", "utf8_drop_last",
	)

	tokenEdge("catalog_state_tokens",
		"state_hash_get", "state_hash_set", "state_index_get", "state_index_set",
		"var_read", "var_write", "save_boot", "zero", "dup", "pop", "not",
		"eq", "ne", "call", "call_cond_static", "call_stack", "again_cond",
		"time_ms", "random_u64", "u64_mod",
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

func (b *blockBuilder) rawBlock(name string, body []byte) [hashSize]byte {
	bodyCopy := append([]byte(nil), body...)
	h := sha256.Sum256(bodyCopy)
	b.blocks = append(b.blocks, builtBlock{name: name, body: bodyCopy, hash: h})
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

func (c *chainBuilder) textUTF8(s string, x, y uint64, r, g, b uint64) {
	c.add("payload_bytes", []byte(s))
	c.pushU64(x)
	c.pushU64(y)
	c.pushColor(r, g, b)
	c.add("surface_text_utf8", nil)
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

func (c *chainBuilder) buttonUTF8(label string, x, y, w, h uint64, r, g, b uint64) {
	c.rect(x, y, w, h)
	c.pushColor(r, g, b)
	c.pushU64(14)
	c.add("surface_round_rect", nil)
	c.rect(x, y, w, h)
	c.pushColor(204, 214, 235)
	c.pushU64(14)
	c.add("surface_round_frame", nil)
	c.textUTF8(label, x+14, y+13, 245, 247, 250)
}

func (c *chainBuilder) rectVars(xVar, yVar, wVar, hVar string) {
	c.varRead(xVar)
	c.varRead(yVar)
	c.varRead(wVar)
	c.varRead(hVar)
	c.add("rect_make", nil)
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

func toyColor(r, g, b uint64) uint64 { return (r & 255) | ((g & 255) << 8) | ((b & 255) << 16) }

func fixedHash(s string) [hashSize]byte { return sha256.Sum256([]byte(s)) }

func toyObjectPayload(typ, x, y, w, h, color, radius, flags uint64, ref, aux [hashSize]byte) []byte {
	out := make([]byte, 128)
	binary.LittleEndian.PutUint64(out[0:8], typ)
	binary.LittleEndian.PutUint64(out[8:16], x)
	binary.LittleEndian.PutUint64(out[16:24], y)
	binary.LittleEndian.PutUint64(out[24:32], w)
	binary.LittleEndian.PutUint64(out[32:40], h)
	binary.LittleEndian.PutUint64(out[40:48], color)
	binary.LittleEndian.PutUint64(out[48:56], radius)
	binary.LittleEndian.PutUint64(out[56:64], flags)
	copy(out[64:96], ref[:])
	copy(out[96:128], aux[:])
	return out
}

func toyStageBlock(noop [hashSize]byte, payloads ...[]byte) []byte {
	out := []byte{}
	for _, payload := range payloads {
		out = append(out, record(noop, payload)...)
	}
	out = append(out, make([]byte, hashSize)...)
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

func uget(c net.Conn, user, key [hashSize]byte) ([hashSize]byte, bool, error) {
	var outHash [hashSize]byte
	body := make([]byte, 0, 64)
	body = append(body, user[:]...)
	body = append(body, key[:]...)
	status, out, err := request(c, 8, body)
	if err != nil {
		return outHash, false, err
	}
	if status == 3 {
		return outHash, false, nil
	}
	if status != 0 {
		return outHash, false, fmt.Errorf("uget status %d", status)
	}
	if len(out) != hashSize {
		return outHash, false, fmt.Errorf("uget returned %d bytes", len(out))
	}
	copy(outHash[:], out)
	return outHash, true, nil
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
