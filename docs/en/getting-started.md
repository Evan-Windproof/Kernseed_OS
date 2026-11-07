# Getting started

Use x86-64 Debian/Ubuntu or WSL2 Ubuntu. setup-host installs GCC, binutils, Make, Git, Python3, xorriso, QEMU and GDB; network access is needed for pinned Limine. GUI needs GTK/WSLg (qemu-system-gui if absent). The kernel itself does not use Linux services.

```sh
git clone https://github.com/Evan-Windproof/Kernseed_OS.git
cd Kernseed_OS
sudo sh scripts/setup-host.sh
make deps
make -j2 iso check
make run
```
Wait for `ks>`, then type `help`, `meminfo`, `tasks`, `apps`, `run hello`, `run counter`. Output is in build/kernseed.elf and build/kernseed.iso. Close the window or interrupt QEMU to exit.

[QEMU](qemu.md) · [VMware](vmware.md) · [Commands](command-reference.md)

[Index](index.md)
