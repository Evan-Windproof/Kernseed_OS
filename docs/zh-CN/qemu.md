# QEMU

默认BIOS/TCG，单核128MiB、std VGA、无网卡。headless仅显示串口，stdin不是键盘。UEFI运行前先将OVMF_VARS_4M.fd复制到/tmp/kernseed-vars.fd，不修改系统模板。Phase7验证过无Secure Boot的OVMF，不代表所有固件。

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
暂停时检查-S或在GDB中continue。缺失Framebuffer会受控panic；GTK错误检查显示后端，缺工具运行setup-host；Limine下载检查网络/CA，不关闭校验。


[Index](index.md)
