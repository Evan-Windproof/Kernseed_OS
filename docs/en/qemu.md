# QEMU

Default is BIOS/TCG, one CPU,128MiB,std VGA, no NIC. Headless shows serial output; stdin is not a keyboard. For UEFI first copy OVMF_VARS_4M.fd to /tmp/kernseed-vars.fd; never write the packaged template. Phase7 validated OVMF without Secure Boot, not every firmware.

```sh
make headless
make debug
# Alternative RAM size; real PS/2 input requires the QEMU window or QMP.
./scripts/run.sh run -m 512M
# Verified Phase7 OVMF path; supply locally installed firmware files:
cp /usr/share/OVMF/OVMF_VARS_4M.fd /tmp/kernseed-vars.fd
./scripts/run.sh headless -m 256M \
 -drive if=pflash,format=raw,unit=0,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
 -drive if=pflash,format=raw,unit=1,file=/tmp/kernseed-vars.fd
```
If paused, remove -S or continue in GDB. Missing framebuffer causes a controlled panic. GTK errors require a display backend; missing tools require setup-host. Limine fetch failures require working network/CA, never disabling checksums.


[Index](index.md)
