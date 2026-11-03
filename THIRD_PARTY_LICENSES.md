# Third-party notices

Kernseed 自有源码使用 [PolyForm Noncommercial 1.0.0](LICENSE)。以下组件保留原许可；
项目不是 OSI Approved Open Source。工具只用于构建或测试，不链接进内核。

## Limine bootloader

- 来源：https://github.com/limine-bootloader/limine
- 固定提交：`aad3edd370955449717a334f0289dee10e2c5f01`（`v8.x-binary`）
- BIOS/UEFI 引导二进制、安装工具：BSD 2-Clause，完整通知见
  [third_party/limine/LICENSE](third_party/limine/LICENSE)。
- `limine.h`：BSD Zero Clause，完整通知保留在该头文件开头。
- 镜像带有 `LIMINE-LICENSE`；不修改第三方二进制。
- 许可允许非商业项目使用和再分发；再分发仍须保留原通知。
- 构建前核对固定 Git 提交与 `third_party/limine/SHA256SUMS`。
  哈希记录从 HTTPS 官方仓库获取的文件身份，不代表独立代码审计或上游签名。

## PolyForm license text

`LICENSE` 原样复制自 PolyForm 官方仓库：
https://github.com/polyformproject/polyform-licenses/blob/76a278c402bc43b8d2b561da140b0f3e17263015/PolyForm-Noncommercial-1.0.0.md

正式页面：https://polyformproject.org/licenses/noncommercial/1.0.0/

## Development tools (not included in the boot image)

- GCC / GNU Binutils / Make / GDB：GNU 工具，各自保留其 GPL 等许可。
- QEMU / SeaBIOS：仅作为模拟器与固件运行，不作为内核实现使用。
- xorriso / libisoburn：创建 ISO，不链接进内核。
- Python 3：执行宿主测试。

工作区内安装的 Debian 软件包含各自 `/usr/share/doc/*/copyright`；
它们不随本仓库源码或 Kernseed ISO 再分发。内核字体由本项目自行绘制，无外部字体依赖。
