#ifndef HEAD_H
#define HEAD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h> 
#include <dirent.h>

void safe_mkdir(const char *path) {
    if (mkdir(path, 0700) < 0) {
        if (errno != EEXIST) {
            perror("error create dir");
            exit(1);
        }
    }
}

void cf(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return;
    
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        fwrite(buffer, 1, bytes, out);
    }

    fclose(in);
    fclose(out);
}

void cd(const char *src_dir, const char *dst_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) return;

    struct dirent *entry;
    char src_path[1024];
    char dst_path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (strcmp(src_dir, ".") == 0 && (strcmp(entry->d_name, "v") == 0 || strcmp(entry->d_name, "main") == 0 || strcmp(entry->d_name, ".v") == 0)) {
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name);

        struct stat statbuf;
        if (stat(src_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                safe_mkdir(dst_path);
                cd(src_path, dst_path);
            } else if (S_ISREG(statbuf.st_mode)) {
                cf(src_path, dst_path);
            }
        }
    }
    closedir(dir);
}

void dd(const char *src_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) return;
    
    struct dirent *entry;
    char src_path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* if (strcmp(src_dir, ".") == 0 && strcmp(entry->d_name, ".v") == 0) {
            continue;
        } */
        if (strcmp(src_dir, ".") == 0 && (strcmp(entry->d_name, "v") == 0 || strcmp(entry->d_name, "main") == 0 || strcmp(entry->d_name, ".v") == 0)) {
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);

        struct stat statbuf;
        if (stat(src_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                dd(src_path);
                rmdir(src_path);
            } else if (S_ISREG(statbuf.st_mode)) {
                remove(src_path);
            }
        }
    }
    closedir(dir);
}

#endif