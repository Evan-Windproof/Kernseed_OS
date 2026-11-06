# 从构建到交互

使用x86-64 Debian/Ubuntu或WSL2 Ubuntu。setup-host安装GCC、binutils、Make、Git、Python3、xorriso、QEMU和GDB；首次获取固定Limine需要网络。图形界面需要GTK/WSLg，缺少时安装qemu-system-gui。内核自身不依赖Linux服务。

```sh
git clone https://github.com/Evan-Windproof/Kernseed_OS.git
cd Kernseed_OS
sudo sh scripts/setup-host.sh
make deps
make -j2 iso check
make run
```
等待`ks>`，输入`help`、`meminfo`、`tasks`、`apps`、`run hello`、`run counter`。产物为build/kernseed.elf和build/kernseed.iso，关闭窗口或中断QEMU即可退出。

[QEMU](qemu.md) · [VMware](vmware.md) · [Commands](command-reference.md)

[Index](index.md)
