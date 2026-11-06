# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
CC := gcc
LD := ld
HOST_CC ?= cc
PYTHON ?= python3
XORRISO ?= xorriso
BUILD ?= build
TEST_FAULT ?= 0
SCHEDULER ?= 1
CONSOLE_SHELL ?= 1
USERMODE ?= 1
ifeq ($(filter $(TEST_FAULT),0 1 2 3 4 5 6 7 8 9 10),)
$(error TEST_FAULT must be 0 (normal), 1 (DE), 2 (UD), 3 (GP), 4 (PF), or 5 (DF), 6 (unmapped), 7 (write), 8 (NX), 9/10 (task guards))
endif
ifneq ($(TEST_FAULT),0)
ifeq ($(BUILD),build)
$(error Fault injection requires a separate BUILD=build/fault-N directory)
endif
endif
CFLAGS := -std=c11 -O2 -g3 -Wall -Wextra -Werror -ffreestanding \
          -fno-builtin -fno-stack-protector -fno-stack-check -fno-pic -fno-pie \
          -fno-asynchronous-unwind-tables -fno-unwind-tables -m64 \
          -march=x86-64 -mno-red-zone -mgeneral-regs-only -mcmodel=kernel \
          -Iinclude -Ithird_party/limine -MMD -MP -DKERNSEED_TEST_FAULT=$(TEST_FAULT) -DKERNSEED_SCHEDULER=$(SCHEDULER) -DKERNSEED_SHELL=$(CONSOLE_SHELL) -DKERNSEED_USERMODE=$(USERMODE)
LDFLAGS := -nostdlib -static -z max-page-size=0x1000 --build-id=none -T boot/linker.ld
C_SOURCES := $(sort $(wildcard kernel/core/*.c kernel/drivers/*.c kernel/arch/x86_64/*.c kernel/interrupts/*.c kernel/memory/*.c kernel/scheduler/*.c kernel/syscall/*.c shell/*.c shell/commands/*.c))
S_SOURCES := $(sort $(wildcard kernel/arch/x86_64/*.S))
OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(C_SOURCES)) $(patsubst %.S,$(BUILD)/%.o,$(S_SOURCES)) $(BUILD)/isr-stubs.o $(BUILD)/apps.o
.PHONY: all deps iso run headless debug test test-host clean check reproducible test-phase2 test-scheduler test-shell
all: iso
.PHONY: FORCE
FORCE:
$(BUILD)/build-config: FORCE
	@mkdir -p $(@D)
	@printf 'test_fault=%s scheduler=%s shell=%s user=%s\n' '$(TEST_FAULT)' '$(SCHEDULER)' '$(CONSOLE_SHELL)' '$(USERMODE)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@
$(BUILD)/isr-stubs.S: scripts/gen-isr.py
	@mkdir -p $(@D)
	$(PYTHON) $< $@
$(BUILD)/isr-stubs.o: $(BUILD)/isr-stubs.S $(BUILD)/build-config Makefile
	$(CC) $(CFLAGS) -c $< -o $@
deps:
	HOST_CC="$(HOST_CC)" ./scripts/fetch-limine.sh
$(BUILD)/%.o: %.c $(BUILD)/build-config Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/%.o: %.S $(BUILD)/build-config Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernseed.elf: $(OBJECTS) boot/linker.ld
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@
iso: $(BUILD)/kernseed.iso
$(BUILD)/kernseed.iso: $(BUILD)/kernseed.elf boot/limine.conf third_party/limine/SHA256SUMS LICENSE THIRD_PARTY_LICENSES.md third_party/limine/LICENSE | deps
	rm -rf $(BUILD)/iso-root
	mkdir -p $(BUILD)/iso-root/boot/limine $(BUILD)/iso-root/EFI/BOOT
	cp $(BUILD)/kernseed.elf $(BUILD)/iso-root/boot/
	cp boot/limine.conf .cache/limine/limine-bios.sys .cache/limine/limine-bios-cd.bin .cache/limine/limine-uefi-cd.bin $(BUILD)/iso-root/boot/limine/
	cp .cache/limine/BOOTX64.EFI $(BUILD)/iso-root/EFI/BOOT/
	cp LICENSE THIRD_PARTY_LICENSES.md $(BUILD)/iso-root/
	cp third_party/limine/LICENSE $(BUILD)/iso-root/LIMINE-LICENSE
	$(XORRISO) -as mkisofs -R -r -J -V KERNSEED \
	    -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
	    --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
	    --protective-msdos-label --modification-date=2026010100000000 \
	    --set_all_file_dates 2026010100000000 \
	    --gpt_disk_guid 6b65726e-7365-4564-8000-000000000001 \
	    -o $@ $(BUILD)/iso-root
	.cache/limine/limine bios-install $@
run headless debug: iso
	KERNSEED_BUILD="$(BUILD)" ./scripts/run.sh $@
$(BUILD)/runtime-test: tests/runtime.c kernel/core/runtime.c include/kernseed/runtime.h
	@mkdir -p $(@D)
	$(HOST_CC) -std=c11 -O2 -Wall -Wextra -Werror -fno-builtin -Iinclude tests/runtime.c kernel/core/runtime.c -o $@
$(BUILD)/framebuffer-test: tests/framebuffer.c kernel/drivers/framebuffer.c include/kernseed/console.h third_party/limine/limine.h
	@mkdir -p $(@D)
	$(HOST_CC) -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -Ithird_party/limine tests/framebuffer.c kernel/drivers/framebuffer.c -o $@
$(BUILD)/memory-span-test: tests/memory-span.c kernel/memory/span.c include/kernseed/memory.h
	@mkdir -p $(@D)
	$(HOST_CC) -std=c11 -O2 -Wall -Wextra -Werror -Iinclude tests/memory-span.c kernel/memory/span.c -o $@
$(BUILD)/shell-test: tests/shell.c shell/parser.c shell/registry.c shell/dispatch.c shell/commands/help.c shell/commands/echo.c include/kernseed/shell.h
	@mkdir -p $(@D)
	$(HOST_CC) -std=c11 -O2 -Wall -Wextra -Werror -Iinclude tests/shell.c shell/parser.c shell/registry.c shell/dispatch.c shell/commands/help.c shell/commands/echo.c -o $@
test-host: $(BUILD)/shell-test $(BUILD)/runtime-test $(BUILD)/framebuffer-test $(BUILD)/memory-span-test
	./$(BUILD)/shell-test
	./$(BUILD)/memory-span-test
	./$(BUILD)/runtime-test
	./$(BUILD)/framebuffer-test
check: $(BUILD)/kernseed.elf
	$(PYTHON) tests/check_elf.py $(BUILD)/kernseed.elf
test: test-host
	$(MAKE) SCHEDULER=0 CONSOLE_SHELL=0 USERMODE=0 iso check
	$(PYTHON) tests/boot.py
	$(PYTHON) tests/phase2.py
	$(PYTHON) tests/phase3.py
	$(MAKE) SCHEDULER=1 CONSOLE_SHELL=0 USERMODE=0 iso check
	$(PYTHON) tests/phase4.py
	$(MAKE) SCHEDULER=1 CONSOLE_SHELL=1 USERMODE=0 iso check
	$(PYTHON) tests/phase5.py
	$(MAKE) USERMODE=1 iso check
	$(PYTHON) tests/phase6.py
test-phase2:
	$(MAKE) SCHEDULER=0 CONSOLE_SHELL=0 USERMODE=0 iso
	$(PYTHON) tests/phase2.py
	$(MAKE) SCHEDULER=1 CONSOLE_SHELL=0 USERMODE=0 iso
test-scheduler:
	$(MAKE) SCHEDULER=1 CONSOLE_SHELL=0 USERMODE=0 iso
	$(PYTHON) tests/phase4.py
	$(MAKE) SCHEDULER=1 CONSOLE_SHELL=1 iso
test-shell: iso
	$(PYTHON) tests/phase5.py
reproducible:
	./scripts/check-reproducible.sh
clean:
	rm -rf "$(BUILD)"
-include config.local.mk
-include $(OBJECTS:.o=.d)

APPS := hello counter probe privileged kernelread kernelwrite badflags badstack
$(BUILD)/apps/%.elf: apps/%.c apps/abi.h apps/linker.ld
	@mkdir -p $(@D)
	$(CC) -std=c11 -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -no-pie -mno-red-zone -mgeneral-regs-only -mcmodel=large -nostdlib -static -Wl,--build-id=none -Wl,-T,apps/linker.ld $< -o $@
$(BUILD)/apps/%.bin: $(BUILD)/apps/%.elf
	objcopy -O binary $< $@
$(BUILD)/apps.S: $(addprefix $(BUILD)/apps/,$(addsuffix .bin,$(APPS)))
	@echo '.section .rodata' > $@
	@for app in $(APPS); do echo ".global app_$${app}_start, app_$${app}_end" >> $@; echo "app_$${app}_start:" >> $@; echo ".incbin \"$(BUILD)/apps/$${app}.bin\"" >> $@; echo "app_$${app}_end:" >> $@; done
	@echo '.section .note.GNU-stack,"",@progbits' >> $@
$(BUILD)/apps.o: $(BUILD)/apps.S
	$(CC) -c $< -o $@

.SECONDARY:
