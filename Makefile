# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
CC := gcc
LD := ld
HOST_CC ?= cc
PYTHON ?= python3
XORRISO ?= xorriso
CFLAGS := -std=c11 -O2 -g3 -Wall -Wextra -Werror -ffreestanding \
          -fno-builtin -fno-stack-protector -fno-stack-check -fno-pic -fno-pie \
          -fno-asynchronous-unwind-tables -fno-unwind-tables -m64 \
          -march=x86-64 -mno-red-zone -mgeneral-regs-only -mcmodel=kernel \
          -Iinclude -Ithird_party/limine -MMD -MP
LDFLAGS := -nostdlib -static -z max-page-size=0x1000 --build-id=none -T boot/linker.ld
C_SOURCES := $(sort $(wildcard kernel/core/*.c kernel/drivers/*.c))
S_SOURCES := kernel/arch/x86_64/entry.S
OBJECTS := $(patsubst %.c,build/%.o,$(C_SOURCES)) $(patsubst %.S,build/%.o,$(S_SOURCES))
.PHONY: all deps iso run headless debug test test-host clean check reproducible
all: iso
deps:
	HOST_CC="$(HOST_CC)" ./scripts/fetch-limine.sh
build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
build/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
build/kernseed.elf: $(OBJECTS) boot/linker.ld
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@
iso: build/kernseed.iso
build/kernseed.iso: build/kernseed.elf boot/limine.conf third_party/limine/SHA256SUMS LICENSE THIRD_PARTY_LICENSES.md third_party/limine/LICENSE | deps
	rm -rf build/iso-root
	mkdir -p build/iso-root/boot/limine build/iso-root/EFI/BOOT
	cp build/kernseed.elf build/iso-root/boot/
	cp boot/limine.conf .cache/limine/limine-bios.sys .cache/limine/limine-bios-cd.bin .cache/limine/limine-uefi-cd.bin build/iso-root/boot/limine/
	cp .cache/limine/BOOTX64.EFI build/iso-root/EFI/BOOT/
	cp LICENSE THIRD_PARTY_LICENSES.md build/iso-root/
	cp third_party/limine/LICENSE build/iso-root/LIMINE-LICENSE
	$(XORRISO) -as mkisofs -R -r -J -V KERNSEED \
	    -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
	    --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
	    --protective-msdos-label --modification-date=2026010100000000 \
	    --set_all_file_dates 2026010100000000 \
	    --gpt_disk_guid 6b65726e-7365-4564-8000-000000000001 \
	    -o $@ build/iso-root
	.cache/limine/limine bios-install $@
run headless debug: iso
	./scripts/run.sh $@
build/runtime-test: tests/runtime.c kernel/core/runtime.c include/kernseed/runtime.h
	@mkdir -p $(@D)
	$(HOST_CC) -std=c11 -O2 -Wall -Wextra -Werror -fno-builtin -Iinclude tests/runtime.c kernel/core/runtime.c -o $@
build/framebuffer-test: tests/framebuffer.c kernel/drivers/framebuffer.c include/kernseed/console.h third_party/limine/limine.h
	@mkdir -p $(@D)
	$(HOST_CC) -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -Ithird_party/limine tests/framebuffer.c kernel/drivers/framebuffer.c -o $@
test-host: build/runtime-test build/framebuffer-test
	./build/runtime-test
	./build/framebuffer-test
check: build/kernseed.elf
	$(PYTHON) tests/check_elf.py build/kernseed.elf
test: iso test-host check
	$(PYTHON) tests/boot.py
reproducible:
	./scripts/check-reproducible.sh
clean:
	rm -rf build
-include config.local.mk
-include $(OBJECTS:.o=.d)
