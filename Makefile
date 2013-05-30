BUILDROOT := $(PWD)
BUILDDIR  := $(BUILDROOT)/build

DIRS      := kernel

AS := nasm
CPP := clang -E
CC  := clang
LD  := i586-elf-ld
DEP := clang -MM
AR  := i586-elf-ar

ASFLAGS := -f elf

CPPFLAGS := -Wall -Wextra -pedantic -m32 -O0 -std=c99 -finline-functions
CPPFLAGS += -fno-stack-protector -ffreestanding -Wno-unused-function
CPPFLAGS += -Wno-unused-parameter -g -Wno-gnu

CCFLAGS  := $(CPPFLAGS) -target i386-pc-linux -mno-sse -mno-mmx

ARFLAGS  := -rc

export BUILDROOT BUILDDIR
export AS CPP CC LD AR
export ASFLAGS CPPFLAGS CCFLAGS LDFLAGS ARFLAGS

.SILENT:
.PHONY: $(DIRS) clean emu default
.DEFAULT: all emu

default: all

all: $(DIRS)

$(DIRS):
	@echo "  \033[35mMAKE\033[0m    " $@
	-@mkdir -p $(BUILDDIR)/$@
	@cd $@; $(MAKE) $(MFLAGS)

clean:
	@for DIR in $(DIRS); do echo "  \033[35mCLEAN\033[0m   " $$DIR; cd $(BUILDROOT)/$$DIR; make clean; done;

emu:
	@echo "\033[35mSTARTING EMULATOR\033[0m"
	@tools/qemu.sh
