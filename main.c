#include "cache.h"

int main(int argc, char *argv[]) {
    char path[4096];
    int number = 0;

    if (argc < 2) {
        /* printf("all commands: i, s\n"); */
		printf("use:\n nix init <- create dir\n nix save <- save project\n nix reset <num> <- go back!\n (shh... don't tell anyone, but if you want your files or folders to be ignored, create a .ni or .gitignore file)\n");
        return 0;
    }

    if (strcmp(argv[1], "init") == 0) {
        safe_mkdir(".nix");
        safe_mkdir(".nix/objects");
        safe_mkdir(".nix/objects/blobs");
        FILE *fp = fopen(".ni", "w");
        if (fp == NULL) {
            printf("maybe next time\n");
            return 1;
        }
        fclose(fp);
    } else if (strcmp(argv[1], "save") == 0) {
        do {
            sprintf(path, ".nix/objects/%d", number);
            number++;
        } while (access(path, F_OK) == 0); 
        
        safe_mkdir(path);
        cd(".", path, "");
        printf("happy end!\n");
    } else if (strcmp(argv[1], "reset") == 0) {
        if (argc >= 3) {
            sprintf(path, ".nix/objects/%s", argv[2]);
        } else {
            printf("please, specify the version number for recovery\n");
            return 1;
        }

        if (access(path, F_OK) != 0) {
            printf("version not found");
            return 1;
        }
        dd(".");
        restore_version(path);
        // cd(path, ".");
    } else {
        printf("i don't know what you're talking about, but good luck\n");
    }

    return 0;
}
