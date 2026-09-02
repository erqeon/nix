#include "cache.h"

int main(int argc, char *argv[]) {
    char path[4096];
    int number = 0;

    if (argc < 2) {
        printf("all commands: i, s\n");
        return 0;
    }

    if (strcmp(argv[1], "i") == 0) {
        safe_mkdir(".v");
        safe_mkdir(".v/objects");
        safe_mkdir(".v/objects/blobs");
    } else if (strcmp(argv[1], "s") == 0) {
        do {
            sprintf(path, ".v/objects/%d", number);
            number++;
        } while (access(path, F_OK) == 0);

        safe_mkdir(path);
        cd(".", path, "");
        printf("happy end!\n");
    } else if (strcmp(argv[1], "r") == 0) {
        if (argc >= 3) {
            sprintf(path, ".v/objects/%s", argv[2]);
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
    } else {
        printf("i don't know what you're talking about, but good luck");
    }

    return 0;
}
