# 命令参考

| Command | 含义 |
|---|---|
| `help [command]` | 注册表自动帮助 |
| `clear` | 清空终端 |
| `about` | 系统信息 |
| `echo [text ...]` | 回显解析参数 |
| `meminfo` | 真实4KiB页统计 |
| `tasks` | TCB快照 |
| `uptime` | PIT tick及整数秒 |
| `reboot` | i8042复位 |
| `apps` | 内置应用列表 |
| `run name` | 同步运行Ring3程序 |

```text
help run
echo "two words"
apps
run hello
run counter
```
未知命令和不合法参数数量会报错，名称区分大小写。privileged/kernelread/kernelwrite/badflags/badstack/probe是故障/安全验证应用。

[Index](index.md)
