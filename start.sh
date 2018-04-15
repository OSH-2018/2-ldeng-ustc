#! /bin/sh
qemu-system-x86_64 -kernel bzImage -initrd initramfs.gz -nographic -append "console=ttyS0"

