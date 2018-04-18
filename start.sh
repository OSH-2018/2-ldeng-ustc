#! /bin/sh
cd rootfs
gcc -static -o init init.c
find . | cpio -oHnewc | gzip > ../initramfs.gz
cd ..
qemu-system-x86_64 -kernel bzImage -initrd initramfs.gz -nographic -append "console=ttyS0"

