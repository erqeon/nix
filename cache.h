#ifndef HEAD_H
#define HEAD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h> 
#include <dirent.h>

#include <openssl/sha.h>
#include <openssl/evp.h> 

void safe_mkdir(const char *path) {
    if (mkdir(path, 0700) < 0) {
        if (errno != EEXIST) {
            perror("error create dir");
            exit(1);
        }
    }
}

int is_ignored(const char *name) {
    if (strcmp(name, ".v") == 0 || strcmp(name, ".vi") == 0) {
        return 0;
    }

    int ignored = 0;

    FILE *vi_file = fopen(".vi", "r");
    if (vi_file) {
        char line[256];
        while (fgets(line, sizeof(line), vi_file)) {
            line[strcspn(line, "\n")] = '\0';
            line[strcspn(line, "\r")] = '\0';

            if (strlen(line) == 0) continue;

            if (strcmp(name, line) == 0) {
                ignored = 1;
                break;
            }      
        }
        fclose(vi_file);
    }

    if (ignored) return 1;

    FILE *git_file = fopen(".gitignore", "r");
    if (git_file) {
        char line[256];
        while (fgets(line, sizeof(line), git_file)) {
            line[strcspn(line, "\n")] = '\0';
            line[strcspn(line, "\r")] = '\0';

            if (strlen(line) == 0) continue;

            if (strcmp(name, line) == 0) {
                ignored = 1;
                break;
            }      
        }
        fclose(git_file);
    }

    return ignored;
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

void cd(const char *src_dir, const char *dst_dir, const char *rel_path) {
    DIR *dir = opendir(src_dir);
    if (!dir) return;

    struct dirent *entry;
    char src_path[1024];
    char dst_path[1024];

    char i_path[1150];
    snprintf(i_path, sizeof(i_path), "%s/index.txt", dst_dir);
    FILE *i_file = fopen(i_path, "a");

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (is_ignored(entry->d_name)) {
            continue;
        }

        if (strcmp(src_dir, ".") == 0 && (strcmp(entry->d_name, "v") == 0 || strcmp(entry->d_name, "main") == 0 || strcmp(entry->d_name, ".v") == 0 || strcmp(entry->d_name, ".vi") == 0 || strcmp(entry->d_name, ".gitignore") == 0)) {
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);

        char new_rel_path[1024];
        if (strlen(rel_path) == 0) {
            snprintf(new_rel_path, sizeof(new_rel_path), "%s", entry->d_name);
        } else {
            snprintf(new_rel_path, sizeof(new_rel_path), "%s/%s", rel_path, entry->d_name);
        }

        struct stat statbuf;
        if (stat(src_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                cd(src_path, dst_dir, new_rel_path);
            } else if (S_ISREG(statbuf.st_mode)) {
                FILE *f = fopen((src_path), "rb");
                if (f) {
                    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
                    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
                    
                    char hash_buf[4096];
                    size_t r_bytes;
                    while ((r_bytes = fread(hash_buf, 1, sizeof(hash_buf), f)) > 0) {
                        EVP_DigestUpdate(mdctx, hash_buf, r_bytes);
                    }
                    
                    unsigned char hash[EVP_MAX_MD_SIZE];
                    unsigned int hash_len;
                    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
                    EVP_MD_CTX_free(mdctx);
                    fclose(f);
                    
                    char sha256_str[65]; 
                    for (unsigned int i = 0; i < hash_len; i++) {
                        sprintf(&sha256_str[i * 2], "%02x", hash[i]);
                    }
                    sha256_str[64] = '\0';

                    char blob_path[1100];
                    snprintf(blob_path, sizeof(blob_path), ".v/objects/blobs/%s", sha256_str);
                    
                    if (access(blob_path, F_OK) != 0) {
                        cf(src_path, blob_path);
                    }
                    
                    if (i_file) {
                        fprintf(i_file, "%s -> %s\n", new_rel_path, sha256_str);
                    }
                }
            }
        }
    }
    closedir(dir);
}

void create_parent_dirs(const char *file_path) {
    char path_copy[1024];
    snprintf(path_copy, sizeof(path_copy), "%s", file_path);
    
    for (int i = 0; path_copy[i] != '\0'; i++) {
        if (path_copy[i] == '/') {
            path_copy[i] = '\0';
            safe_mkdir(path_copy);
            path_copy[i] = '/';
        }
    }
}

void restore_version(const char *version_dir) {
    char i_path[1150];
    snprintf(i_path, sizeof(i_path), "%s/index.txt", version_dir);
    
    FILE *i_file = fopen(i_path, "r");
    if (!i_file) {
        printf("Error: index.txt not found in %s\n", version_dir);
        return;
    }

    char line[1200];
    while (fgets(line, sizeof(line), i_file)) {
        char filename[1024];
        char hash_str[65];
        
        if (sscanf(line, "%s -> %s", filename, hash_str) == 2) {
            char blob_path[1100];
            char target_path[1100];
            
            snprintf(blob_path, sizeof(blob_path), ".v/objects/blobs/%s", hash_str);
            snprintf(target_path, sizeof(target_path), "./%s", filename);

            create_parent_dirs(target_path);
            
            cf(blob_path, target_path);
        }
    }
    fclose(i_file);
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

        if (is_ignored(entry->d_name)) {
            continue;
        }

        if (strcmp(src_dir, ".") == 0 && (strcmp(entry->d_name, "v") == 0 || strcmp(entry->d_name, "main") == 0 || strcmp(entry->d_name, ".v") == 0 || strcmp(entry->d_name, ".vi") == 0)) {
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