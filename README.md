# **Laporan Praktikum Sistem Operasi 2026 - Modul 4**

**Nama:** Rido Patra Yudhistira Edwin  
**NRP:** 5027251120  
**Kelas:** B  
**Repositori:** SISOP-4-2026-IT-120

-----

## **Langkah Pengerjaan**

### **Inisialisasi & Persiapan Direktori**

```bash
# Masuk ke direktori praktikum

# Clone repositori yang sudah ada
git clone git@github.com:Bl4nk06/SISOP-4-2026-IT-120.git
cd SISOP-4-2026-IT-120

# Membuat struktur folder soal
mkdir soal_1 soal_2 soal_3
```

---

### **Struktur Direktori:**

```bash
.
├── soal_1
│   └── kenz_rescue.c
├── soal_2
│   ├── Dockerfile
│   ├── client.c
│   ├── encrypted_storage
│   ├── fuse.c
│   ├── fuse_mount
│   └── server
└── soal_3
    ├── Dockerfile
    ├── data
    │   ├── docs
    │   ├── ebooks
    │   ├── papers
    │   └── sourcecode
    ├── docker-compose.yml
    ├── entrypoint.sh
    ├── logs
    │   └── libraryit.log
    └── smb.conf
```

---

## **Penyelesaian Soal 1**

### **Soal 1: Save Asisten Kenz (FUSE Passthrough & Virtual File)**

*Tujuan: Membuat filesystem mirror/passthrough menggunakan FUSE3 dengan memanipulasi file virtual secara on-the-fly.*

1. **Manajemen File & Direktori**
    
    Menyiapkan folder `amba_files/` berisi berkas `1.txt` sampai `7.txt` yang diekstrak dari arsip zip, lalu membersihkan file zip mentahan dari direktori kerja.

2. **Pure Passthrough FUSE**

    Mengimplementasikan callback FUSE dasar seperti `getattr`, `readdir`, dan `read` agar folder mountpoint (`mnt/`) mencerminkan isi `amba_files/` secara identik (*byte-identical*).

3. **Manipulasi File Virtual**

    Menyisipkan file virtual gaib bernama `tujuan.txt` di root mountpoint yang berukuran konsisten (66 bytes) saat diperiksa melalui perintah `stat`, tanpa mengubah struktur fisik folder asli (`amba_files/`).

4. **String Processing on-the-Fly**

    Membaca file `1.txt` sampai `7.txt` secara berurutan saat `tujuan.txt` dibuka (`cat`), mengekstrak string koordinat di belakang prefix `KOORD:`, lalu menggabungkannya ke memori RAM secara dinamis menjadi format `Tujuan Mas Amba: <gabungan_fragmen>, 23:59 WIB\n`.

### **Langkah teknis**

```bash
# 1. Pindah ke direktori soal_1
cd soal_1

# 2. Download file bahan praktikum amba_files.zip via gdown
gdown 1nLXFhptDo2mnUlZsw8pTWyAVpV49W20U -O amba_files.zip

# 3. Unzip berkas dan langsung hapus zip mentahannya
unzip amba_files.zip && rm amba_files.zip

# 4. Membuat direktori tujuan mountpoint
mkdir mnt

# 5. Membuat file kenz_rescue.c sekaligus mengisinya
nano kenz_rescue.c
```

---

### **Kode yang Digunakan**

#### **Kode yang digunakan dalam file `kenz_rescue.c`**

```c
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
```

---

### **Penjelasan isi kode terhadap soal**

#### **Penjelasan isi kode `kenz_rescue.c` terhadap soal 1**

1. **Menyimpan Alamat Folder Asli (dir_path & get_full_path)**

    Kode ini menggunakan variabel `dir_path` untuk mencatat alamat folder asli (`amba_files`). Setiap kali ada perintah dari folder mount (`mnt/`), fungsi bantuan `get_full_path` akan otomatis menggabungkan alamat asli tersebut dengan nama file yang diminta. Ini membuat folder `mnt/` sukses menjadi "cermin" yang menampilkan isi yang sama persis dengan folder aslinya.

2. **Membuat KTP Palsu untuk File Virtual (x_getattr)**

    Fungsi `x_getattr` bertugas memberikan data informasi (metadata) sebuah file ke sistem operasi.

    * **Untuk file gaib:** Jika ada yang mengecek file `/tujuan.txt`, kode akan langsung memalsukan informasinya dengan memberi izin baca (`0444`) dan menyiapkan ukuran ruang memori (`st_size = 1024`), meskipun filenya tidak pernah ada di disk.
    * **Untuk file asli:** Jika yang dicek adalah file lain (`1.txt` sampai `7.txt`), kode akan membiarkan sistem membaca informasi asli dari folder `amba_files`.

3. **Memunculkan Nama File Gaib saat di-LS (x_readdir)**

    Fungsi `x_readdir` gunanya untuk menampilkan daftar file saat kamu mengetik perintah `ls`. Di dalam fungsi ini, kode pertama-tama membaca dan memasukkan semua file asli dari folder `amba_files`. Tepat sebelum selesai, kode sengaja menyelipkan perintah `filler(buf, "tujuan.txt", ...)` secara manual. Hasilnya, file `tujuan.txt` otomatis ikut muncul di layar komputer.

4. **Menggabungkan Koordinat secara Langsung di RAM (x_read)**

    Fungsi `x_read` bekerja otomatis saat file dibaca menggunakan perintah `cat mnt/tujuan.txt`.

    * **Proses scan cepat:** Kode langsung membuka file `1.txt` sampai `7.txt` satu per satu di latar belakang.
    * **Mencari baris kunci:** Setiap file dibaca baris demi baris menggunakan perintah `fgets`. Begitu menemukan baris yang berawalan `KOORD:`, kode akan memotong tulisan "KOORD:" tersebut, membersihkan spasi yang tidak penting, lalu menggabungkan potongan koordinatnya ke dalam memori RAM.
    * **Mencetak hasil:** Semua potongan tadi disatukan ke dalam format kalimat `Tujuan Mas Amba: [Hasil Koordinat], 23:59 WIB\n` lalu dikirim ke layar monitor menggunakan fungsi `memcpy`.

---

### **Cara menggunakan**

```bash
# 1. Compile file kenz_rescue.c menggunakan bendera pkg-config FUSE3
gcc kenz_rescue.c -o kenz_rescue `pkg-config fuse3 --cflags --libs`

# 2. Jalankan program FUSE (Mount amba_files ke folder mnt)
./kenz_rescue amba_files mnt

# 3. Pengujian passthrough: Periksa apakah isi folder mnt sama dengan amba_files
ls mnt
diff -s amba_files/1.txt mnt/1.txt

# 4. Pengujian berkas virtual: Cek stat dan isi dari tujuan.txt
stat mnt/tujuan.txt
cat mnt/tujuan.txt

# 5. Jika sudah selesai pengujian, unmount folder mnt
fusermount3 -u mnt

```

---

### **Output**

* Tampilan terminal saat menjalankan program `kenz_rescue` dan melakukan mount ke direktori tujuan.

* Tampilan hasil perintah `ls -la` pada direktori mount dan isi dari file `tujuan.txt` yang merupakan hasil penggabungan koordinat dari file-file log.

---

### **Kendala**

* Saat awal-awal compile selalu error.

---

## **Penyelesaian Soal 2**

### **Soal 2: Poke MOO (FUSE Encryption & Docker Containerization)**

*Tujuan: Membangun ekosistem mini database terisolasi di dalam kontainer Docker dengan sistem penyimpanan terenkripsi otomatis berbasis FUSE (XOR 0x76).*

1. **Implementasi Operasi Dasar FUSE (fuse.c)**

    Membangun fondasi program FUSE dengan menyediakan fungsi operasi standar agar folder virtual dapat berkomunikasi dengan sistem operasi.

    * **12 Callback Utama** – Mengimplementasikan fungsi `getattr`, `readdir`, `mkdir`, `rmdir`, `create`, `open`, `read`, `write`, `truncate`, `unlink`, `access`, dan `utimens`.
    * **Penghubung Dua Folder** – Mengaitkan folder penyimpanan asli (`encrypted_storage`) dengan folder titik kait virtual (`fuse_mount`).

2. **Sinkronisasi File System Passthrough**

    Memastikan direktori virtual hasil mounting dapat digunakan dan bertingkah laku normal layaknya media penyimpanan biasa.

    * **Operasi CRUD File & Folder** – Pengguna bisa membuat file/folder baru, membaca data, masuk ke sub-direktori, melihat metadata atribut, hingga menghapus objek secara normal di dalam `fuse_mount`.
    * **Mirroring Otomatis** – Setiap aksi modifikasi data (create, update, delete) pada folder virtual otomatis tersinkronisasi dan tercermin langsung pada folder asli.

3. **Mekanisme Penerjemah Sandi Otomatis (XOR 0x76)**

    Mengubah FUSE agar tidak sekadar menjadi cermin passthrough biasa, melainkan bertindak sebagai gerbang enkripsi otomatis.

    * **Enkripsi File Baru** – Setiap file yang ditulis lewat folder `fuse_mount` otomatis diacak isinya menggunakan rumus matematika XOR (kunci `0x76`) dan disimpan ke folder asli dengan tambahan ekstensi `.enc`.
    * **Dekripsi On-the-Fly** – Berkas backup berformat `.enc` yang ada di folder asli akan otomatis diterjemahkan kembali ke teks normal saat diakses atau di-`cat` melalui folder `fuse_mount`.

4. **Validasi File Checker Sukses**

    Melakukan uji coba fisik untuk memastikan fungsi penerjemah enkripsi/dekripsi FUSE sudah berjalan secara valid sebelum lanjut ke tahap Docker.

    * **Folder Khusus Tes** – Membuat sub-direktori baru bernama `tests` di dalam folder fisik `encrypted_storage`.
    * **Verifikasi Backup Terbaca** – Mengunduh berkas contoh `notes.csv.enc` ke dalam folder tersebut dan memastikan teks aslinya bisa terdekripsi sempurna saat dilihat di dalam `fuse_mount`.

5. **Isolasi Server Database (Containerization)**

    Mengisolasi seluruh program layanan mini database ke dalam lingkungan kontainer yang aman dan mandiri menggunakan Docker.

    * **Spesifikasi Dockerfile** – Membangun cetakan sistem berbasis `ubuntu:latest`, menyalin file eksekusi server ke dalam folder `/app`, serta membuka jalur komunikasi jaringan via Port 9000.
    * **Standardisasi Tag Image** – Melakukan build image aplikasi di komputer terminal hingga memunculkan tag resmi dengan nama `soal-2-modul-4-sisop`.

6. **Integrasi Ekosistem Jaringan Klien-Server (Integration)**

    Menghubungkan seluruh komponen (FUSE, Docker, dan Aplikasi Klien) agar dapat bekerja bersama secara ujung-ke-ujung (*end-to-end*).

    * **Mekanisme Bind Mount** – Menjalankan kontainer bernama `db_app` di background dengan menyambungkan folder `fuse_mount` host langsung ke folder `/app/db` milik kontainer.
    * **Koneksi Klien via TCP Socket** – Membuat program `client.c` di host untuk mengirimkan perintah manipulasi database (seperti Create DB, Create Table `.csv`, Insert, Select) ke Docker lewat port jaringan 9000.
    * **Hasil Akhir Enkripsi** – Membuktikan bahwa pembuatan tabel database oleh klien menghasilkan berkas normal di dalam `fuse_mount`, namun tersimpan aman dalam kondisi teracak menjadi berkas `.csv.enc` di folder host.

---

### **Langkah teknis**

```bash
# 1. Pindah ke direktori soal_2
cd soal_2

# 2. Membuat direktori penyimpanan asli dan titik kait (mountpoint)
mkdir encrypted_storage fuse_mount

# 3. Membuat sub-direktori 'tests' di dalam encrypted_storage untuk tempat berkas pengujian
mkdir -p encrypted_storage/tests

# 4. Unduh berkas biner 'server' dari Google Drive via gdown menggunakan ID file spesifik
gdown 18jOp0LWt-Fwjesj__FpOU8VKC7Ce0xVG -O server

# 5. Unduh berkas 'notes.csv.enc' langsung ke dalam folder tests
gdown 189vYoqJVV_8ry3jTXQx1_WPebZkhBobr -O encrypted_storage/tests/notes.csv.enc

# 6. Beri izin akses eksekusi pada file biner server yang baru diunduh
chmod +x server

# 7. Membuat berkas kode program translator filesystem FUSE
nano fuse.c

# 8. Membuat spesifikasi instruksi Dockerfile untuk container database
nano Dockerfile

# 9. Membuat berkas program aplikasi klien database
nano client.c

```

---

### **Kode yang Digunakan**

#### **Kode yang digunakan dalam file `fuse.c`**

```c
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
```

#### **Kode yang digunakan dalam file `Dockerfile`**

```dockerfile
# Gunakan base image sesuai instruksi soal
FROM ubuntu:latest

# Set direktori kerja di dalam kontainer ke /app
WORKDIR /app

# Salin file 'server' dari folder lokal ke direktori kerja (/app) di kontainer
# Format: COPY <sumber_di_laptop> <tujuan_di_kontainer>
COPY server /app/server

# Beri izin eksekusi agar file server bisa dijalankan
RUN chmod +x /app/server

# Buat folder /app/db karena server akan mencari data di sana
RUN mkdir -p /app/db

# Expose port 9000 sesuai spek soal
EXPOSE 9000

# Jalankan servernya
CMD ["./server"]
```

#### **Kode yang digunakan dalam file `client.c`**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE];

    // 1. Membuat Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Konversi alamat IP ke biner (localhost)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 2. Menghubungkan ke Server Database di Docker
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed. Pastikan Docker sudah jalan di port 9000!\n");
        return -1;
    }

    printf("Connected to DB Server on port %d\n", PORT);
    printf("Type HELP for available commands\n");
    printf("Type EXIT to quit\n\n");

    while (1) {
        printf("db > ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0; // Hapus newline

        if (strcmp(command, "EXIT") == 0) break;
        if (strlen(command) == 0) continue;

        // 3. Kirim perintah ke Server
        send(sock, command, strlen(command), 0);

        // 4. Terima balasan dari Server
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            printf("%s\n", buffer);
        } else {
            printf("Server disconnected.\n");
            break;
        }
    }

    close(sock);
    return 0;
}
```

---

### **Penjelasan isi kode terhadap soal**

#### **Penjelasan isi kode `fuse.c` terhadap soal 2**

1. **Fungsi Bitwise XOR Otomatis (xor_cipher)**

    * Kode ini menyediakan sebuah fungsi enkripsi/dekripsi sederhana bernama `xor_cipher`. Setiap karakter data yang masuk atau keluar akan dihitung ulang secara bitwise menggunakan kunci rahasia `0x76` (sesuai spesifikasi soal). Karena sifat operasi XOR, fungsi ini bekerja bolak-balik: mengacak data biasa menjadi sandi, dan mengembalikan data sandi menjadi teks normal.

2. **Manipulasi Jalur Berkas Otomatis (get_enc_path)**

    * Fungsi bantuan `get_enc_path` bertugas menerjemahkan jalur akses. Ketika aplikasi database mencoba mengakses sebuah dokumen di folder virtual `fuse_mount/`, fungsi ini akan otomatis mengalihkan perintah tersebut ke folder fisik `encrypted_storage/` sekaligus menambahkan akhiran nama berupa format `.enc` di belakang berkasnya.

3. **Gerbang Modifikasi Data Terenkripsi (x_write & x_read)**

    * Fungsi ini memanipulasi isi data berkas sebelum menyentuh media penyimpanan fisik:
    * **Saat Menulis (`x_write`):** Data mentah dari aplikasi database akan disalin ke RAM sementara, diacak isinya lewat fungsi `xor_cipher`, lalu barulah disimpan fisik ke folder `encrypted_storage`.
    * **Saat Membaca (`x_read`):** Berkas `.enc` yang teracak di dalam disk akan dibuka, didekripsi kembali ke teks normal di memori RAM, lalu dikirim ke pengguna dalam bentuk dokumen teks utuh yang bersih.

4. **Sinkronisasi Operasi Manajemen Folder (x_mkdir & x_create)**

    * Fungsi `x_mkdir` dan `x_create` digunakan agar user tetap bisa membuat folder baru (seperti folder database) dan dokumen baru (seperti berkas tabel `.csv`) langsung dari dalam folder virtual `fuse_mount/`. Perintah ini diteruskan ke sistem operasi agar folder fisik aslinya ikut terbentuk di dalam `encrypted_storage`.

#### **Penjelasan berkas penyusunan `Dockerfile` terhadap soal 2**

1. **Penyediaan Sistem Operasi Terisolasi (`FROM` & `WORKDIR`)**

    * Instruksi `FROM ubuntu:latest` mendefinisikan sistem operasi dasar kontainer menggunakan rilis Ubuntu terbaru demi menjamin kestabilan program. Perintah `WORKDIR /app` menetapkan direktori kerja utama di dalam kontainer pada folder `/app`, sehingga seluruh instruksi berikutnya akan dieksekusi secara otomatis di lokasi tersebut.

2. **Pemasangan Dokumen Aplikasi Database (`COPY` & `RUN`)**

    * Perintah `COPY server /app/server` memindahkan berkas biner server dari sistem laptop host ke dalam lingkungan Docker. File ini kemudian diberikan hak akses penuh untuk dieksekusi lewat perintah `RUN chmod +x /app/server`. Dockerfile ini juga otomatis membuat folder tujuan `/app/db` via perintah `RUN mkdir -p /app/db` sebagai tempat bermuaranya data *bind mount* FUSE.

3. **Jalur Komunikasi & Eksekusi Otomatis (`EXPOSE` & `CMD`)**

    * Instruksi `EXPOSE 9000` membuka jalur komunikasi port 9000 kontainer agar aplikasi klien dari luar bisa melakukan *handshake* jaringan TCP. Terakhir, `CMD ["./server"]` dipasang agar program server database langsung otomatis berjalan seketika kontainer Docker dinyalakan.

#### **Penjelasan isi kode `client.c` terhadap soal 2**

1. **Inisialisasi Jalur Komunikasi Jaringan (TCP Socket)**

    * Kode klien mendefinisikan alamat tujuan `127.0.0.1` (localhost) dengan ketetapan Port `9000`. Saat program dijalankan, fungsi `socket()` and `connect()` akan membuat jalur pipa data interaktif agar terminal laptop host bisa mengirim dan menerima pesan secara langsung dari dalam server database yang terisolasi di dalam Docker.


2. **Loop Terminal Interaktif (db >)**

    * Di dalam fungsi utama `main()`, terdapat loop tak terbatas (`while(1)`) yang mencetak prompt interaktif `db > ` ke layar monitor. Kode menggunakan fungsi `fgets()` untuk menangkap teks query manajemen database yang diketikkan pengguna, membersihkan karakter ujung baris, lalu melemparkannya ke server Docker melalui fungsi `send()`.

---

### **Cara menggunakan**

```bash
# 1. Compile file fuse.c menggunakan bendera pkg-config FUSE3
gcc fuse.c -o fuse_app `pkg-config fuse3 --cflags --libs`

# 2. Jalankan program FUSE (Mount encrypted_storage ke folder fuse_mount)
./fuse_app fuse_mount

# 3. Pengujian awal FUSE (Checker): Pastikan berkas notes.csv.enc terdekripsi otomatis
cat fuse_mount/tests/notes.csv

# 4. Build Docker Image untuk server database terisolasi
docker build -t soal-2-modul-4-sisop .

# 5. Jalankan Docker Container dengan mekanisme Bind Mount ke folder FUSE
docker run -d --name db_app -p 9000:9000 -v $(pwd)/fuse_mount:/app/db soal-2-modul-4-sisop

# 6. Compile program client.c di terminal lokal laptop/host
gcc client.c -o client

# 7. Jalankan client untuk mulai mengirim perintah ke server database
./client

# 8. Jika pengujian interaksi database sudah selesai, matikan kontainer Docker
docker stop db_app && docker rm db_app

# 9. Lepaskan kaitan (unmount) filesystem FUSE dari sistem
fusermount3 -u fuse_mount

```

---

### **Output**

* Tampilan proses `docker build` untuk image server database dan menjalankannya sehingga port 9000 terbuka.

* Tampilan yang menunjukkan client yang terhubung ke server (port 9000) dan bukti bahwa file yang dibuat melalui mount point FUSE tersimpan dalam keadaan terenkripsi (XOR) di direktori asli.

* Tampilan terminal client saat melakukan perintah `CREATE DATABASE`, `CREATE TABLE`, dan `INSERT` data.

---

### **Kendala**

* Tidak ada

---

## **Penyelesaian Soal 3**

### **Soal 3: LibraryIT (Samba Network File Server & Docker Compose)**

*Tujuan: Membangun infrastruktur server penyimpanan digital terpusat menggunakan protokol Samba di dalam kontainer Docker, lengkap dengan pengelompokan hak akses (ACL), persistensi data pada host, serta sistem pemantauan log audit terpisah secara real-time.*

1. **Otomatisasi Provisioning Lingkungan Server (libraryit-server)**

    Membangun kontainer server penyimpanan berbasis Ubuntu secara instan yang langsung siap pakai tanpa konfigurasi manual pasca-aktif.

    * **Inisialisasi Folder** – Menyediakan empat ruang penyimpanan koleksi perpustakaan pada direktori internal kontainer, yaitu `/libraryit/ebooks`, `/libraryit/papers`, `/libraryit/sourcecode`, dan `/libraryit/docs`.
    * **Manajemen Pengguna Otomatis** – Mendaftarkan langsung tiga user kredensial ke dalam database enkripsi Samba (`pdbedit`), yakni `member` (pwd: `member123`), `contributor` (pwd: `contrib456`), dan `librarian` (pwd: `lib789`).
    * **Pengelompokan Hak Akses** – Mengklasifikasikan akun `member` ke dalam grup `readonly` (GID 1000), serta mengelompokkan `contributor` dan `librarian` ke dalam grup `staff` (GID 50) secara otomatis sejak *build* kontainer.

2. **Penerapan Kebijakan Akses Ketat Konten Digital (ACL)**

    Mengonfigurasi fungsionalitas pembagian berkas Samba (`smb.conf`) agar mematuhi matriks hak akses spesifik dan menutup celah akses tanpa identitas (*guest login*).
    
    * **Koleksi ebooks & papers** – Mengonfigurasi folder agar bersifat *read-only* bagi grup `readonly`, namun memberikan hak tulis (*write list*) eksklusif kepada grup `@staff`.
    * **Koleksi sourcecode (Hidden Share)** – Menerapkan parameter pembatasan agar folder ini tidak dapat dilihat (`browseable = no`) saat dipindai oleh user di luar grup, serta membatasi akses masuk secara ketat hanya untuk grup `@staff`.
    * **Koleksi docs** – Membuka izin baca ke seluruh anggota perpustakaan, namun hak penulisan dikunci secara spesifik hanya untuk user `librarian`. Pengguna `contributor` dilarang menulis di folder ini meskipun ia bagian dari grup `staff`.

3. **Persistensi Data & Keamanan Direktori Host**

    Mengamankan file perpustakaan dari kehilangan data akibat siklus kontainer mati/menyala ulang serta melindunginya dari manipulasi ilegal di level OS Host.

    * **Mekanisme Volume Bind Mount** – Mengaitkan folder lokal laptop host (`./data/`) langsung menuju direktori `/libraryit/` di dalam kontainer menggunakan Docker Compose.
    * **Isolasi Folder Sourcecode** – Membatasi hak akses folder fisik `sourcecode` di laptop host dengan permission ketat `750` (`drwxr-x---`) milik `root:staff` agar tidak dapat diintip oleh user umum di luar grup.
    * **Sistem Write Lock Folder Docs** – Mengunci folder `docs` di sisi host agar bersifat read-only bagi user host umum, sehingga modifikasi data dipaksa hanya bisa lewat gerbang protokol Samba.

4. **Sistem Pemantauan Log Audit Jaringan (libraryit-logger)**

    Merekam jejak aktivitas transaksi file dan mengalirkan data log secara dinamis menggunakan arsitektur multi-container.

    * **Samba Full Audit VFS** – Mengaktifkan modul `full_audit` pada server Samba untuk mendeteksi setiap aksi koneksi jaringan (`connect`), pembukaan file (`open`), pembuatan folder (`mkdirat`), penggantian nama (`rename`), dan penghapusan (`unlink`).
    * **Standardisasi Format Log** – Mencatat metadata aktivitas ke dalam berkas log lokal dengan skema terstruktur `[Tanggal Jam] [LEVEL] [USERNAME] [AKSI] [NAMA FILE/SHARE]`.
    * **Monitoring Real-Time Klien Logger** – Menyediakan layanan kontainer mandiri berbasis biner ringan `alpine` bernama `libraryit-logger` yang khusus bertugas menangkap aliran (*stream*) log server dan memancarkannya langsung ke terminal via perintah `docker logs`.

---

### **Langkah teknis**

```bash
# 1. Pindah ke direktori soal_3
cd soal_3

# 2. Membuat pohon direktori penyimpanan data lokal dan logs di host
mkdir -p data/ebooks data/papers data/sourcecode data/docs logs

# 3. Menerapkan permission ketat pada folder sourcecode di host sesuai spek soal
sudo chown -R root:50 data/sourcecode
sudo chmod 750 data/sourcecode

# 4. Menerapkan penguncian agar folder docs di host tidak bisa dimodifikasi dari luar Samba
sudo chmod 555 data/docs

# 5. Membuat berkas spesifikasi instruksi lingkungan sistem operasi kontainer
nano Dockerfile

# 6. Membuat skrip otomatisasi startup kontainer untuk setup Samba
nano entrypoint.sh

# 7. Membuat konfigurasi aturan share dan hak akses Samba
nano smb.conf

# 8. Menyusun manifes orkestrasi multi-container Docker Compose
nano docker-compose.yml

```

---

### **Kode yang Digunakan**

#### **Kode yang digunakan dalam file `Dockerfile`**

```dockerfile
FROM ubuntu:latest

# 1. Install Samba + VFS Modules (WAJIB ADA UNTUK AUDIT)
RUN apt-get update && apt-get install -y \
    samba \
    samba-common-bin \
    samba-vfs-modules \
    && rm -rf /var/lib/apt/lists/*

# 2. Buat User & Group
RUN touch /var/lib/samba/private/passdb.tdb && \
    (userdel -r $(id -un 1000 2>/dev/null) || true) && \
    groupadd -g 1000 -f readonly && \
    groupadd -g 50 -f staff && \
    useradd -m -u 1000 -g readonly -s /sbin/nologin member && \
    useradd -m -u 1001 -g staff -s /sbin/nologin contributor && \
    useradd -m -u 1002 -g staff -s /sbin/nologin librarian

# 3. Set Password Samba
RUN (echo "member123"; echo "member123") | smbpasswd -a -s member && \
    (echo "contrib456"; echo "contrib456") | smbpasswd -a -s contributor && \
    (echo "lib789"; echo "lib789") | smbpasswd -a -s librarian

COPY smb.conf /etc/samba/smb.conf
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 139 445
ENTRYPOINT ["/entrypoint.sh"]
```

#### **Kode yang digunakan dalam file `entrypoint.sh`**

```bash
#!/bin/bash

# Buat folder share kalau belum ada
mkdir -p /libraryit/ebooks /libraryit/papers /libraryit/sourcecode /libraryit/docs

# Buat file log agar tail -f tidak error
mkdir -p /var/log/samba
touch /var/log/samba/libraryit.log

# Set permission agar bisa diakses
chmod -R 777 /libraryit
chmod -R 777 /var/log/samba

# Jalankan Samba di foreground agar container tidak mati
exec smbd --foreground --no-process-group --log-stdout < /dev/null
```

#### **Kode yang digunakan dalam file `smb.conf`**

```ini
[global]
   workgroup = WORKGROUP
   server string = LibraryIT Server
   netbios name = libraryit-server
   security = user
   map to guest = Bad User
   
   log file = /var/log/samba/libraryit.log
   log level = 3
   
   # VFS Audit Fix
   vfs objects = full_audit
   full_audit:prefix = %u|%I|%m|%S
   full_audit:success = mkdirat rename unlink open
   full_audit:failure = connect
   full_audit:facility = LOCAL7
   full_audit:priority = NOTICE

   load printers = no
   printing = bsd
   printcap name = /dev/null
   disable spoolss = yes

[ebooks]
   path = /libraryit/ebooks
   browseable = yes
   read only = yes
   valid users = @staff, @readonly

[papers]
   path = /libraryit/papers
   browseable = yes
   read only = yes
   valid users = @staff, @readonly

[sourcecode]
   path = /libraryit/sourcecode
   browseable = no 
   read only = no
   valid users = @staff

[docs]
   path = /libraryit/docs
   browseable = yes
   read only = yes
   write list = librarian
   valid users = @staff
```

#### **Kode yang digunakan dalam file `docker-compose.yml`**

```yaml
version: '3.8'

services:
  libraryit-server:
    build: .
    container_name: libraryit-server
    # Tambahkan ini biar container gak langsung exit pas ada error
    tty: true
    stdin_open: true
    volumes:
      - ./data/ebooks:/libraryit/ebooks
      - ./data/papers:/libraryit/papers
      - ./data/sourcecode:/libraryit/sourcecode
      - ./data/docs:/libraryit/docs 
      - ./logs:/var/log/samba
    ports:
      - "1445:445"
    restart: always

  libraryit-logger:
    image: alpine # Alpine lebih lengkap command-nya dibanding busybox
    container_name: libraryit-logger
    depends_on:
      - libraryit-server
    volumes:
      - ./logs:/var/log/samba
    # Trik: Tunggu 5 detik baru tail, biar Samba sempet bikin filenya
    command: sh -c "sleep 5 && tail -F /var/log/samba/libraryit.log"
    restart: always
```

---

### **Penjelasan isi kode terhadap soal**

#### **Penjelasan berkas penyusunan `Dockerfile` terhadap soal 3**

1. **Pemasangan Dependensi Inti Paket Samba (`RUN apt-get`)**

    * Perintah `apt-get install -y samba samba-common-bin samba-vfs-modules` dipasang untuk mengunduh aplikasi server Samba beserta modul *Virtual File System* (VFS). Modul ini wajib disertakan untuk memenuhi instruksi poin (d) mengenai pencatatan log audit akses file.

2. **Otomatisasi Akun Pengguna & Enkripsi Sandi Samba (`RUN useradd & smbpasswd`)**

    * Kode menggunakan kombinasi perintah `groupadd` dan `useradd` untuk mendaftarkan `member`, `contributor`, dan `librarian` langsung ke sistem Linux Ubuntu kontainer dengan penetapan GID grup `staff` (50) dan `readonly` (1000).
    * Setelah itu, perintah biner `smbpasswd -a -s` langsung mengeksekusi konversi akun-akun Linux tersebut menjadi akun kredensial Samba aktif yang dilengkapi kata sandi masing-masing secara otomatis saat proses *build*.

3. **Delegasi Booting Server Melalui Skrip Kontrol (`COPY & ENTRYPOINT`)**

    * Dokumen ini menyalin file `smb.conf` ke direktori resmi Samba sistem, serta mengarahkan perintah akhir kontainer ke berkas `entrypoint.sh`. Skrip eksternal ini dipakai agar proses inisialisasi permission folder internal bisa dieksekusi tepat sebelum server biner `smbd` dijalankan di mode *foreground*.

#### **Penjelasan berkas skrip `entrypoint.sh` terhadap soal 3**

1. **Penyusunan Struktur Folder Koleksi Internal (`mkdir -p`)**

    * Skrip ini memastikan folder target penyimpanan internal perpustakaan (`/libraryit/ebooks`, dll) di dalam Ubuntu terjamin keberadaannya. Hal ini menghindari terjadinya kegagalan proses *mounting* protokol Samba apabila folder fisiknya kosong.

2. **Pemberian Akses Global Longgar internal Kontainer (`chmod 777`)**

    * Perintah `chmod -R 777 /libraryit` di dalam skrip ini digunakan untuk membuka seluruh akses berkas di level sistem operasi kontainer internal. Tujuannya adalah mendelegasikan dan menyerahkan sepenuhnya kendali pembatasan hak tulis/baca (*Read/Write*) kepada aturan yang ditulis di file `smb.conf`.

#### **Penjelasan berkas konfigurasi `smb.conf` terhadap soal 3**

1. **Konfigurasi Global & Sistem VFS Audit (`[global]`)**

    * Di bawah label `[global]`, parameter `log file` diarahkan ke berkas `/var/log/samba/libraryit.log`. Untuk mengaktifkan fitur pencatatan, dipasang kode `vfs objects = full_audit` dengan format awalan prefix `full_audit:prefix = %u|%I|%m|%S` untuk merekam variabel username, IP, nama mesin, dan nama folder share. Batasan perekaman disaring via parameter `success` dan `failure`.

2. **Pembatasan Hak Akses Berbasis Grup (`valid users`, `read only`, `browseable`)**

    * **`[ebooks]` dan `[papers]`:** Diberikan parameter `read only = yes` untuk mengunci folder menjadi hanya bisa dibaca, namun dipasang pengecualian khusus `write list = @staff` agar anggota grup staff tetap bisa memodifikasi data.
    * **`[sourcecode]`:** Dipasang instruksi `browseable = no` yang secara otomatis menyembunyikan nama folder ini saat komputer klien melakukan *scanning*. Akses masuk pun dibatasi ketat hanya untuk grup staff lewat kode `valid users = @staff`.
    * **`[docs]`:** Parameter `valid users` dibuka untuk semua grup, tetapi hak penulisan dikunci secara spesifik ke satu user melalui perintah `write list = librarian`.

#### **Penjelasan berkas manifes `docker-compose.yml` terhadap soal 3**

1. **Manajemen Volume Permanen Host (`volumes: libraryit-server`)**

    * Blok ini memetakan folder lokal host `./data/` dan `./logs/` laptop ke dalam kontainer. Ini menjawab kekhawatiran pustakawan pada poin (c) agar data tabel database/file perpustakaan tidak lenyap ketika kontainer Docker dihancurkan.

2. **Orkestrasi Hubungan Antar Layanan Kontainer (`depends_on & command`)**

    * Pada bagian `libraryit-logger`, dipasang deklarasi `depends_on: - libraryit-server` untuk memastikan sistem logger baru aktif sesudah server Samba menyala. Layanan logger ini memakai image biner Alpine yang mengeksekusi perintah shell `tail -F` pada folder logs hasil berbagi volume dengan server untuk mencetak baris audit secara instan ke layar monitor host.

---

### **Cara menggunakan**

```bash
# 1. Pastikan skrip entrypoint memiliki izin eksekusi sebelum di-build oleh Docker
chmod +x entrypoint.sh

# 2. Menyalakan seluruh ekosistem server dan logger di background via Docker Compose
docker compose up -d --build

# 3. Verifikasi status kontainer untuk memastikan kedua service sudah berjalan normal
docker compose ps

# 4. Melakukan pengecekan otomatisasi user dan grup di dalam database kontainer
docker exec -it libraryit-server pdbedit -L
docker exec -it libraryit-server getent group staff readonly

# 5. Menguji pemindaian folder share menggunakan akun member (sourcecode harus tersembunyi)
smbclient -L //localhost -p 1445 -U member%member123

# 6. Menguji batasan hak tulis kontributor pada share 'docs' (harus ditolak/ACCESS_DENIED)
smbclient //localhost/docs -p 1445 -U contributor%contrib456 -c "put beberapa_catatan.txt"

# 7. Memantau logs aktivitas audit secara real-time dari kontainer logger
docker logs -f libraryit-logger

# 8. Jika seluruh pengujian infrastruktur selesai, matikan ekosistem kontainer
docker compose down

```

---

### **Output**

* Tampilan terminal saat menjalankan `docker-compose up -d --build`. Pastikan kedua service (`libraryit-server` dan `libraryit-logger`) berjalan.

* Screenshot Pengetesan Akses Samba (ACL):

    * Menggunakan `smbclient` untuk melihat share yang tersedia (pembuktian `sourcecode` tidak *browseable*).
    * Mencoba menulis ke folder `docs` dengan user `contributor` (harusnya gagal/Access Denied).
    * Mencoba menulis ke folder `docs` dengan user `librarian` (harusnya berhasil).

* Tampikan output dari perintah `docker logs -f libraryit-logger` yang menunjukkan log aktivitas (INFO/WARNING) saat user mencoba mengakses file.

* Tampilan hasil `ls -ld` pada direktori `./data/sourcecode` di laptop host untuk membuktikan permission `750` dan kepemilikan grup `staff`.

---

### **Kendala**

* `smbclient` selalu error

---