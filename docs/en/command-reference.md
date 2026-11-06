# Command reference

| Command | Meaning |
|---|---|
| `help [command]` | Registry-generated help |
| `clear` | Clear terminal |
| `about` | Kernel information |
| `echo [text ...]` | Parsed arguments |
| `meminfo` | Actual4KiB page statistics |
| `tasks` | TCB snapshot |
| `uptime` | PIT ticks and integer seconds |
| `reboot` | i8042 reset |
| `apps` | Built-in whitelist |
| `run name` | Synchronous Ring3 execution |

```text
help run
echo "two words"
apps
run hello
run counter
```
Unknown names and invalid argument counts are rejected. Names are case sensitive. privileged/kernelread/kernelwrite/badflags/badstack/probe are intentional diagnostic apps, not general applications.

[Index](index.md)
