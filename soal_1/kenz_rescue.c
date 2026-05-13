#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

static char dir_path[1024];

void get_full_path(char *fpath, const char *path) {
    snprintf(fpath, 2048, "%s%s", dir_path, path);
}

static int x_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi; 
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (strcmp(path, "/tujuan.txt") == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024; 
        return 0;
    }
    char fpath[2048];
    get_full_path(fpath, path);
    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    char fpath[2048];
    get_full_path(fpath, path);
    DIR *dp = opendir(fpath);
    if (dp == NULL) return -errno;
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0)) break;
    }
    if (strcmp(path, "/") == 0) {
        filler(buf, "tujuan.txt", NULL, 0, 0);
    }
    closedir(dp);
    return 0;
}

static int x_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    if (strcmp(path, "/tujuan.txt") == 0) {
        char final_content[1024] = "Tujuan Mas Amba: ";
        char coords[512] = "";
        for (int i = 1; i <= 7; i++) {
            char fpath[2048], line[256];
            snprintf(fpath, sizeof(fpath), "%s/%d.txt", dir_path, i);
            FILE *f = fopen(fpath, "r");
            if (f) {
                while (fgets(line, sizeof(line), f)) {
                    if (strncmp(line, "KOORD:", 6) == 0) {
                        char *p = line + 6;
                        while (*p == ' ' || *p == '\t') p++;
                        strtok(p, "\r\n");
                        strcat(coords, p);
                        break;
                    }
                }
                fclose(f);
            }
        }
        strcat(final_content, coords);
        strcat(final_content, ", 23:59 WIB\n");
        size_t len = strlen(final_content);
        if (offset < (off_t)len) {
            if (offset + size > len) size = len - offset;
            memcpy(buf, final_content + offset, size);
        } else size = 0;
        return (int)size;
    }
    char fpath[2048];
    get_full_path(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;
    int res = pread(fd, buf, size, offset);
    close(fd);
    return res;
}

static struct fuse_operations x_oper = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .read    = x_read,
};

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    char absolute_path[1024];
    if (realpath(argv[argc-2], absolute_path)) {
        strncpy(dir_path, absolute_path, sizeof(dir_path) - 1);
        dir_path[sizeof(dir_path) - 1] = '\0';
    }
    argv[argc-2] = argv[argc-1];
    argc--;
    return fuse_main(argc, argv, &x_oper, NULL);
}