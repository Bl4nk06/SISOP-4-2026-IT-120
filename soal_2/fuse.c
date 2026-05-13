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

static const char *DIR_ASLI = "/home/bl4nk06/ITS/Praktikum/Semester_2/Sistem_Operasi/SISOP-4-2026-IT-120/soal_2/encrypted_storage";
const char KEY = 0x76;

// Fungsi XOR untuk enkripsi/dekripsi isi file
void xor_cipher(char *data, size_t n) {
    for (size_t i = 0; i < n; i++) {
        data[i] ^= KEY;
    }
}

// Helper untuk mengubah path mnt -> encrypted_storage (tambah .enc)
void get_enc_path(char *fpath, const char *path) {
    if (strcmp(path, "/") == 0) {
        sprintf(fpath, "%s", DIR_ASLI);
    } else {
        // Folder tidak perlu .enc (sesuai contoh soal), hanya file
        // Namun untuk mempermudah, kita asumsikan file saja yang .enc
        // Jika folder juga di-enc, logika akan lebih kompleks
        sprintf(fpath, "%s%s.enc", DIR_ASLI, path);
    }
}

static int x_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[2048];
    get_enc_path(fpath, path);
    
    int res = lstat(fpath, stbuf);
    if (res == -1) {
        // Coba cek tanpa .enc (siapa tahu itu folder)
        sprintf(fpath, "%s%s", DIR_ASLI, path);
        res = lstat(fpath, stbuf);
        if (res == -1) return -errno;
    }
    return 0;
}

static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    char fpath[2048];
    sprintf(fpath, "%s%s", DIR_ASLI, path);
    
    DIR *dp = opendir(fpath);
    if (dp == NULL) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        char name[256];
        strcpy(name, de->d_name);
        
        // Hilangkan akhiran .enc saat ditampilkan di fuse_mount
        char *ext = strstr(name, ".enc");
        if (ext) *ext = '\0';

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, name, &st, 0, 0)) break;
    }
    closedir(dp);
    return 0;
}

static int x_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[2048];
    get_enc_path(fpath, path);

    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res != -1) {
        xor_cipher(buf, res); // Dekripsi saat dibaca
    }
    close(fd);
    return res;
}

static int x_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[2048];
    get_enc_path(fpath, path);

    char *temp_buf = malloc(size);
    memcpy(temp_buf, buf, size);
    xor_cipher(temp_buf, size); // Enkripsi sebelum ditulis

    int fd = open(fpath, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        free(temp_buf);
        return -errno;
    }

    int res = pwrite(fd, temp_buf, size, offset);
    free(temp_buf);
    close(fd);
    return res;
}

static int x_mkdir(const char *path, mode_t mode) {
    char fpath[2048];
    sprintf(fpath, "%s%s", DIR_ASLI, path);
    int res = mkdir(fpath, mode);
    if (res == -1) return -errno;
    return 0;
}

static int x_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[2048];
    get_enc_path(fpath, path);
    int res = creat(fpath, mode);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static struct fuse_operations x_oper = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .read    = x_read,
    .write   = x_write,
    .mkdir   = x_mkdir,
    .create  = x_create,
    // Tambahkan rmdir, unlink, truncate, dll dengan pola yang sama
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &x_oper, NULL);
}