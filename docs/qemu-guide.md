# QEMU 操作指南

在仓库根目录先执行 `make`。默认使用 TCG，不依赖 `/dev/kvm` 或云主机嵌套虚拟化。

```sh
make headless             # 串口到当前终端，Ctrl-C 结束
make run                  # GTK 窗口，窗口关闭/终端 Ctrl-C 结束
make debug                # -S 暂停，GDB 127.0.0.1:1234
```

默认配置：`pc,accel=tcg`、`qemu64`、128 MiB、1 CPU、std VGA、无网卡、ISO CD 启动。
`-no-reboot -no-shutdown` 有助于保留故障现场，但进程仍在不能证明启动正确。
当前内核关闭中断且不读取键盘，启动后没有 Shell。

GUI 需要相应 QEMU 图形包，例如 Ubuntu 的 `qemu-system-gui`，以及可用 DISPLAY/Wayland。
WSL 2 推荐 WSLg；没有图形服务时使用 headless，不能把无图形环境误诊为内核启动失败。
云环境可通过自动测试的 QMP `screendump` 得到实际虚拟屏幕 `screen.ppm`。

```sh
make test
find build/test-results -name results.json
```

测试通过本地 UNIX socket 控制 QMP/GDB，不暴露外部调试端口。受限云沙箱可能禁止创建
socket；这需要执行权限，不能关闭测试断言来绕过。串口日志保存在每个测试子目录。
每次测试使用独立进程与目录，失败保留日志，结束只关闭自己启动的 QEMU。

手动初步排错可运行：

```sh
./scripts/run.sh headless -d int,guest_errors -D build/qemu-debug.log
```

调试日志可能很大，仅在需要时启用。无启动消息时先检查 `build/kernseed.iso`、
Limine 的路径和软件包 firmware；宿主 QEMU 找不到 BIOS 是环境问题。
`make clean && make -j2 && make test` 可从零重建；不会删除 `.cache/`。

当前验收覆盖 SeaBIOS，镜像中的 UEFI 文件尚未使用 OVMF 验证；没有实机启动保证。
