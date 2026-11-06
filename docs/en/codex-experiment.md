# Codex development experiment

Codex implemented kernel modules, build scripts, QEMU/GDB tests and documentation. The founder defined scope, reviewed/merged changes, made release decisions and reports QEMU/VMware acceptance. Automated tests inspect real registers, stack frames and page tables; successful boot alone is insufficient. Phase7 records120s preemption, four RAM sizes and repeated application cleanup; those measurements are historical, not newly executed here. Two test-fixture mistakes (early resource sampling and int return width) were corrected with failed logs retained. Git branch push worked while PR GraphQL returned Forbidden. Release metadata query also returned Forbidden in this documentation run; no release tag/version is invented. This is one bounded experiment, not evidence of universally autonomous kernel development.

[Evidence](../evidence/phase-7/README.md) · [Report](../phase7-test-report.md)

[Index](index.md)
