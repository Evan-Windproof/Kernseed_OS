# Third-party notices

Kernseed 项目自有源码采用 [GNU GPL 第 3 版或任何后续版本](LICENSE)
（`GPL-3.0-or-later`），允许商业与非商业使用，但必须遵守 GPL。
以下第三方组件保留各自原许可证，不因本次迁移而重新许可。
工具只用于构建或测试，不链接进内核。

## Limine bootloader

- 来源：https://github.com/limine-bootloader/limine
- 固定提交：`aad3edd370955449717a334f0289dee10e2c5f01`（`v8.x-binary`）
- BIOS/UEFI 引导二进制、安装工具：BSD 2-Clause，完整通知见
  [third_party/limine/LICENSE](third_party/limine/LICENSE)。
- `limine.h`：BSD Zero Clause，完整通知保留在该头文件开头。
- 镜像带有 `LIMINE-LICENSE`；不修改第三方二进制。
- BSD 许可允许商业和非商业使用与再分发；再分发仍须保留适用的原通知。
- 构建前核对固定 Git 提交与 `third_party/limine/SHA256SUMS`。
  哈希记录从 HTTPS 官方仓库获取的文件身份，不代表独立代码审计或上游签名。

## Project license text and historical licensing

`LICENSE` 是完整、未经修改的 GNU GPLv3 正式文本，取自 GNU GCC 的官方 GitHub 镜像：
https://github.com/gcc-mirror/gcc/blob/5115c7e447fc07457443df874bf57840e8316d5f/COPYING3

GNU 正式页面：https://www.gnu.org/licenses/gpl-3.0.html

项目授权为 `GPL-3.0-or-later`；“或任何后续版本”的授权见 README 和各自有源文件的 SPDX 标识，
不通过修改 GPL 正文表达。该正文属于 Free Software Foundation，保留其原有版权与复制声明。

历史版本使用 PolyForm Noncommercial 1.0.0；本次迁移不撤销历史 Release 的原有许可。
历史提交中的原始 `LICENSE` 和阶段报告保留，来源记录见[迁移审查](docs/license-migration.md)。
HoofTTY 是基于 Kernseed OS 的派生项目；本次提交仅处理 Kernseed OS。

## Development tools (not included in the boot image)

- GCC / GNU Binutils / Make / GDB：GNU 工具，各自保留其 GPL 等许可。
- QEMU / SeaBIOS：仅作为模拟器与固件运行，不作为内核实现使用。
- xorriso / libisoburn：创建 ISO，不链接进内核。
- Python 3：执行宿主测试。

工作区内安装的 Debian 软件包含各自 `/usr/share/doc/*/copyright`；
它们不随本仓库源码或 Kernseed ISO 再分发。内核字体由本项目自行绘制，无外部字体依赖。
