SUBDIR := kernel
DIRS := $(shell find . -type d)

TARGET := $(BUILDDIR)/$(SUBDIR)/kernel.elf

VPATH := ./ $(DIRS)

SOURCES := boot/kinit.c boot/boot.s
SOURCES += $(shell find . -name "*.s" | grep -v boot.s)
SOURCES += $(shell find . -name "*.c" | grep -v kinit.c)

OBJECTS := $(patsubst %.s,%.o,$(SOURCES))
OBJECTS := $(patsubst %.c,%.o,$(OBJECTS))
OBJECTS := $(addprefix $(BUILDDIR)/$(SUBDIR)/,$(notdir $(OBJECTS)))

CCFLAGS += -Iinclude 
CPPFLAGS += -Iinclude
LDFLAGS += -T $(BUILDROOT)/$(SUBDIR)/include/Link.ld

LIBGCC_LDFLAGS += -lgcc -L/usr/local/cross/lib/gcc/i586-elf/4.6.4 

SCANTABLE := ../tools/en_US.scantable
SCANTABLE_GEN := include/scantable.inc

all: $(TARGET)

$(TARGET): scantable $(OBJECTS) 
	@echo "   \033[32mln\033[0m     " $(TARGET)
	@$(LD) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBPATH) $(LIBGCC_LDFLAGS)

$(BUILDDIR)/$(SUBDIR)/%.o: %.c
	@echo "  \033[33m gcc\033[0m    " $<
	@$(CC) $(CCFLAGS) -c $< -o $@

$(BUILDDIR)/$(SUBDIR)/%.o: %.s
	@echo "   \033[33mnasm\033[0m   " $<
	@$(AS) $(ASFLAGS) $< -o $@

scantable:
	@echo "   \033[33mscantable\033[0m   "
	@python ../tools/scantable.py $(SCANTABLE) $(SCANTABLE_GEN)

clean:
	-@rm $(OBJECTS) 2>/dev/null
	-@rm $(TARGET) 2>/dev/null
	-@rm include/scantable.inc 2>/dev/null
