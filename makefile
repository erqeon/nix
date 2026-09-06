run: main.c
	gcc main.c -o nix -lcrypto
	./nix

del:
	rm -rf nix
