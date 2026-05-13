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