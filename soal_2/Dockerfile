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