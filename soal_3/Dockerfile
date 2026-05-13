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