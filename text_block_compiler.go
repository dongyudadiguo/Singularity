package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"

	"toy/blocktext"
)

func main() {
	mapPath := flag.String("map", "mods_map.txt", "token map path")
	outPath := flag.String("out", "", "output block path; valid with one input")
	outDir := flag.String("out-dir", "", "output directory for one or more inputs")
	writeHash := flag.Bool("hash", true, "write a companion .hash file")
	flag.Parse()

	inputs := flag.Args()
	if len(inputs) == 0 {
		fmt.Fprintln(os.Stderr, "usage: go run ./text_block_compiler.go [-map mods_map.txt] [-out file.blk|-out-dir dir] source.cvm.txt [...]")
		os.Exit(2)
	}
	if *outPath != "" && len(inputs) != 1 {
		fmt.Fprintln(os.Stderr, "-out can only be used with one input file")
		os.Exit(2)
	}

	tokens, err := blocktext.ReadTokenMap(*mapPath)
	must(err)

	for _, input := range inputs {
		body, err := blocktext.CompileFile(input, tokens)
		must(err)

		out := outputPath(input, *outPath, *outDir)
		must(os.MkdirAll(filepath.Dir(out), 0755))
		must(os.WriteFile(out, body, 0644))

		h := blocktext.HashBytes(body)
		if *writeHash {
			must(os.WriteFile(out+".hash", []byte(fmt.Sprintf("%x\n", h)), 0644))
		}
		fmt.Printf("compiled\t%s\t%s\t%x\n", input, out, h)
	}
}

func outputPath(input, explicit, dir string) string {
	if explicit != "" {
		return explicit
	}
	name := filepath.Base(input) + ".blk"
	if dir != "" {
		return filepath.Join(dir, name)
	}
	return input + ".blk"
}

func must(err error) {
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
