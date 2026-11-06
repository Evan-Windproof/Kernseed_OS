# VMware Workstation

发起人报告已通过VMware基本启动和交互验收；本轮未运行VMware，不能确定具体支持版本/虚拟硬件矩阵。建议复验：创建Other 64-bit虚拟机，单CPU、256MiB–1GiB内存，将build/kernseed.iso挂载为已连接CD/DVD，先选BIOS并从光盘启动。点击客户机窗口输入help和run hello。无VMware Tools、网络或磁盘安装。键盘/显示失败时记录固件和控制器配置，与QEMU比较；这里不认证UEFI/虚拟USB组合。

[Index](index.md)
