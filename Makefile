BUILD := build
CC := gcc
LD := ld
PY := python3
CFLAGS := -m32 -std=gnu11 -O2 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin -mno-sse -mno-mmx -msoft-float -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -Ikernel -Iinclude -Ithird_party/freetype/include
KERNEL_OBJS := $(BUILD)/bios.o $(addprefix $(BUILD)/kernel/,$(addsuffix .o,main lib hardware video display settings assets appui services ttfont font_runtime audio cdrom hda sfx music mp3 storage media installer fs loader shell desktop power rtl8139 pcnet))
LWIP_SRC := $(wildcard third_party/lwip/src/core/*.c third_party/lwip/src/core/ipv4/*.c) third_party/lwip/src/netif/ethernet.c
LWIP_OBJS := $(patsubst third_party/lwip/src/%.c,$(BUILD)/lwip/%.o,$(LWIP_SRC))
NETFLAGS := $(CFLAGS) -Ikernel/network/include -Ithird_party/lwip/src/include
KERNEL_OBJS += $(BUILD)/kernel/network.o $(LWIP_OBJS)
APPS := hello calc editor sysinfo game player puzzle
RUNS := $(addprefix $(BUILD)/apps/,$(addsuffix .run,$(APPS)))

.PHONY: all legacy grub installer doom nes freedoom clean test test-ui test-doom test-nes test-install test-storage test-audio test-desktop test-network test-internet test-upgrade test-resolutions test-display-settings desktop-data run-legacy run-grub
all: legacy grub
.PHONY: x64 test-x64 test-gaming
x64:
	$(PY) tools/build_x64.py
test-x64: x64
	$(PY) tests/x64.py
test-gaming: grub
	$(PY) tests/gaming_performance.py
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/boot16.bin: legacy/boot.asm | $(BUILD)
	nasm -f bin $< -o $@
$(BUILD)/kernel16.bin: legacy/kernel.asm | $(BUILD)
	nasm -f bin $< -o $@
$(BUILD)/hello16.run: legacy/hello.asm | $(BUILD)
	nasm -f bin $< -o $@
$(BUILD)/legacy.img: $(BUILD)/boot16.bin $(BUILD)/kernel16.bin $(BUILD)/hello16.run tools/floppy.py
	$(PY) tools/floppy.py create $@
legacy: $(BUILD)/legacy.img
$(BUILD)/entry.o: kernel/entry.asm | $(BUILD)
	nasm -f elf32 $< -o $@
$(BUILD)/bios.o: kernel/bios.asm | $(BUILD)
	nasm -f elf32 $< -o $@
$(BUILD)/kernel/%.o: kernel/%.c kernel/os.h kernel/storage.h kernel/edid.h kernel/ui_font.h include/bds.h | $(BUILD)
	mkdir -p $(BUILD)/kernel
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel/mp3.o: kernel/mp3.c third_party/minimp3/minimp3.h kernel/os.h include/bds.h | $(BUILD)
	mkdir -p $(BUILD)/kernel
	$(CC) $(filter-out -msoft-float,$(CFLAGS)) -mfpmath=387 -Idoom/include -c $< -o $@
$(BUILD)/kernel/network.o: kernel/network.c kernel/tls.h kernel/os.h include/bds.h kernel/network/include/lwipopts.h kernel/network/include/arch/cc.h | $(BUILD)
	mkdir -p $(BUILD)/kernel
	$(CC) $(NETFLAGS) -c $< -o $@
$(BUILD)/lwip/%.o: third_party/lwip/src/%.c kernel/network/include/lwipopts.h kernel/network/include/arch/cc.h kernel/os.h include/bds.h | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(NETFLAGS) -c $< -o $@
TLS_SRC := $(shell find third_party/bearssl/src -name '*.c' -o -name '*.h') $(wildcard third_party/bearssl/inc/*.h) kernel/tls.c kernel/tls.h kernel/tls_anchors.h tools/build_tls.py
$(BUILD)/tls/libtls.a: $(TLS_SRC)
	$(PY) tools/build_tls.py
$(BUILD)/kernel.elf: $(BUILD)/entry.o $(KERNEL_OBJS) $(BUILD)/tls/libtls.a $(BUILD)/font32/libfont.a kernel/link.ld
	$(LD) -m elf_i386 -T kernel/link.ld -o $@ $(BUILD)/entry.o $(KERNEL_OBJS) $(BUILD)/tls/libtls.a $(BUILD)/font32/libfont.a $(shell $(CC) -m32 -print-libgcc-file-name)
	grub-file --is-x86-multiboot2 $@
$(BUILD)/bds-os.iso: $(BUILD)/kernel.elf grub/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/
	cp grub/grub.cfg $(BUILD)/iso/boot/grub/
	grub-mkrescue -o $@ $(BUILD)/iso
$(BUILD)/apps/%.o: apps/%.c apps/common.h include/bds.h | $(BUILD)
	mkdir -p $(BUILD)/apps
	$(CC) $(CFLAGS) -Wno-unused-function -ffunction-sections -c $< -o $@
$(BUILD)/apps/%.run: $(BUILD)/apps/%.o apps/link.ld tools/runpack.py $(wildcard assets/icons/*.ICO)
	$(LD) -m elf_i386 -T apps/link.ld -o $(BUILD)/apps/$*.elf $<
	objcopy -O binary $(BUILD)/apps/$*.elf $(BUILD)/apps/$*.bin
	$(PY) tools/runpack.py $(BUILD)/apps/$*.elf $(BUILD)/apps/$*.bin $@
DOOM_SRC := $(wildcard third_party/doomgeneric/doomgeneric/*.c third_party/doomgeneric/doomgeneric/*.h doom/*.c doom/include/*.h)
$(BUILD)/apps/doom.run: $(DOOM_SRC) tools/build_doom.py tools/runpack.py include/bds.h apps/link.ld | $(BUILD)
	mkdir -p $(BUILD)/apps
	$(PY) tools/build_doom.py
doom: $(BUILD)/apps/doom.run
NES_SRC := $(wildcard third_party/infones/src/*.cpp third_party/infones/src/*.h third_party/infones/src/mapper/*.cpp nes/*.cpp)
UTILITY_SRC := $(wildcard third_party/lodepng/*.c third_party/lodepng/*.h third_party/gumbo/src/*.c third_party/gumbo/src/*.h) $(wildcard third_party/elk/*.c third_party/elk/*.h) tools/runpack.py apps/browser_style.h apps/browser_script.h apps/utility_libc.c doom/libc.c doom/include/libc.h tools/build_utilities.py include/bds.h apps/link.ld
$(BUILD)/apps/image.run: apps/image.c $(UTILITY_SRC) | $(BUILD)
	$(PY) tools/build_utilities.py image
$(BUILD)/apps/browser.run: apps/browser.c apps/browser_number.c $(UTILITY_SRC) | $(BUILD)
	$(PY) tools/build_utilities.py browser
MEDIA_SRC := $(shell find third_party/libdvdnav/src third_party/libdvdread/src media/compat -type f) $(wildcard media/*.c media/*.h third_party/libmpeg2/libmpeg2/*.c third_party/libmpeg2/libmpeg2/*.h third_party/libmpeg2/include/*.h third_party/liba52/liba52/*.c third_party/liba52/liba52/*.h third_party/liba52/include/*.h third_party/pl_mpeg/*.h)
$(BUILD)/apps/player.run: $(MEDIA_SRC) tools/build_media.py tools/runpack.py doom/libc.c doom/include/libc.h include/bds.h apps/link.ld | $(BUILD)
	$(PY) tools/build_media.py
.PHONY: media test-media
media: $(BUILD)/apps/player.run

test-media: grub
	$(PY) tests/media.py
$(BUILD)/apps/nes.run: $(NES_SRC) tools/build_nes.py tools/runpack.py include/bds.h apps/link.ld | $(BUILD)
	$(PY) tools/build_nes.py
nes: $(BUILD)/apps/nes.run
CLASSICUBE_SRC := $(wildcard third_party/classicube/src/*.c third_party/classicube/src/*.h classicube/*.c classicube/*.h)
$(BUILD)/apps/cube.run: $(CLASSICUBE_SRC) doom/libc.c doom/include/libc.h tools/build_classicube.py tools/runpack.py include/bds.h apps/link.ld assets/icons/CUBE.ICO | $(BUILD)
	$(PY) tools/build_classicube.py
.PHONY: classicube test-classicube
classicube: $(BUILD)/apps/cube.run
test-classicube: grub
	$(PY) tests/classicube.py
freedoom:
	$(PY) tools/freedoom.py
$(BUILD)/seed.img: $(RUNS) $(BUILD)/apps/doom.run $(BUILD)/apps/nes.run $(BUILD)/apps/cube.run $(BUILD)/apps/image.run $(BUILD)/apps/browser.run tools/disk.py tools/desktop_assets.py $(wildcard assets/icons/*.ICO assets/sounds/*.WAV) assets/branding/README.md assets/branding/xp-pack-readme.txt docs/MODERN-PLATFORM.md docs/FONTS.md docs/COMPATIBILITY.md docs/GAMING.md third_party/dejavu/LICENSE third_party/dejavu/DejaVuSansMono.ttf third_party/freetype/docs/FTL.TXT assets/BOOT.BMP assets/WALLPAPR.BMP assets/wallpaper-source.png assets/HOME.HTM assets/DEMO.MP3 docs/HTTPS.md docs/MEDIA.md third_party/libmpeg2/COPYING third_party/liba52/COPYING third_party/pl_mpeg/LICENSE docs/DESKTOP-AUDIO.md docs/NETWORK-DESKTOP.md docs/DESKTOP-UPGRADE.md docs/GETTING-STARTED.md docs/CREDITS.md docs/CHANGELOG.md licenses/NOTICE.md licenses/AGPL-3.0.txt docs/CLASSICUBE.md third_party/classicube/misc/cc_textures.zip
	$(PY) tools/disk.py create $@
$(BUILD)/data.img: | $(BUILD)/seed.img
	cp $(BUILD)/seed.img $@
grub: $(BUILD)/bds-os.iso $(BUILD)/seed.img $(BUILD)/data.img
installer: grub
	$(PY) tools/installer.py --data $(BUILD)/data.img
test-install: installer
	$(PY) tests/install.py
	$(PY) tests/install_safety.py
test-storage: grub
	$(PY) tests/storage.py
test-audio: grub
	$(PY) tests/audio.py
test-desktop: grub
	$(PY) tests/desktop_features.py
test-network: grub
	$(PY) tests/network_desktop.py
test-internet: grub
	$(PY) tests/network_internet.py
test-upgrade: grub
	$(CC) -O2 -Ithird_party/elk tests/elk_limits.c third_party/elk/elk.c -o $(BUILD)/elk-limits
	$(BUILD)/elk-limits
	$(PY) tests/desktop_upgrade.py
test-display-settings: grub
	$(PY) tests/display_settings.py
.PHONY: test-settings-tabs
test-settings-tabs: grub
	$(PY) tests/settings_tabs.py
test-resolutions: grub
	$(PY) tests/resolutions.py
desktop-data: grub
	$(PY) tools/desktop_assets.py $(BUILD)/data.img
run-legacy: legacy
	qemu-system-i386 -drive file=$(BUILD)/legacy.img,format=raw,if=floppy -serial stdio
run-grub: grub
	qemu-system-i386 -m 128 -cdrom $(BUILD)/bds-os.iso -drive file=$(BUILD)/data.img,format=raw,if=ide,index=0 -serial stdio
test: all
	$(CC) -std=gnu11 -O2 -Iinclude -Wno-unused-function -Wno-unused-variable tests/game_logic.c -o $(BUILD)/game-logic-test
	$(BUILD)/game-logic-test
	$(PY) tests/integrity.py
	$(PY) tests/boot.py
test-ui: all
	$(PY) tests/interactive.py
test-doom: all
	$(PY) tests/interactive.py doom
test-nes: all
	$(PY) tests/nes.py
clean:
	rm -rf $(BUILD)

$(BUILD)/font32/libfont.a: $(shell find third_party/freetype/src third_party/freetype/include kernel/fontconfig -type f) tools/build_freetype.py
	$(PY) tools/build_freetype.py 32

$(RUNS) $(BUILD)/apps/doom.run $(BUILD)/apps/nes.run $(BUILD)/apps/cube.run $(BUILD)/apps/image.run $(BUILD)/apps/browser.run: $(wildcard assets/icons/*.ICO)
