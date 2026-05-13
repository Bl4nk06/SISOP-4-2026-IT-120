FROM ubuntu:latest

# Install dependensi (nc untuk testing, library fuse jika diperlukan)
RUN apt-get update && apt-get install -y netcat-openbsd

# Set workdir
WORKDIR /app

# Copy server binary (asumsi sudah dicompile)
COPY server /app/server

# Expose port 9000
EXPOSE 9000

# Jalankan server
CMD ["./server"]