Instruction token semantics must grow from low-level, general, platform-neutral primitives toward higher-level, specialized behavior.

Do not jump levels. Do not add a specialized instruction if it can be easily composed from existing lower-level instructions.

Token blocks are different: token blocks may freely compose behavior at any level, including application-specific behavior such as a graphical boot editor.

The id.bin in the directory is the verified identity

server.go is the source code that has been deployed to the server with IP address 118.25.42.70

first_boot.c publishes a first boot block. By default it builds and publishes a minimal windowed first boot program:

```
gcc -std=c99 -O2 -o first_boot.exe first_boot.c -lws2_32 -ladvapi32
.\first_boot.exe --dry-run
.\first_boot.exe
```

Use `--hash <64hex>` to publish an existing local cache block as the first boot entry instead. The full toy-house boot editor source is available at `cmd/boot_editor_builder` and can be rebuilt with Go when a Go toolchain is installed.
