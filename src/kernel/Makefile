TARGET_ASMFLAGS += -f elf64

# Compiler Flags
# -mno-red-zone:  Disable the "red zone" (128 bytes below stack pointer) because interrupts can clobber it.
# -mcmodel=kernel: Use the kernel code model (absolute addressing in upper 2GB or negative 2GB).
# -ffreestanding: No standard library.
TARGET_CFLAGS = -ffreestanding -m64 -g -c -I. -mno-red-zone -mno-mmx -mcmodel=kernel -Wno-attributes #src/cjson
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T src/kernel/linker.ld -z max-page-size=0x1000 -nostdlib

HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SOURCES_C = $(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			./math.c \
			./libs/cjson/cJSON.c \
			$(wildcard */*/*/*.c) \
			arch/i686/isrs_gen.c

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \
			  arch/i686/isrs_gen.inc

SOURCES_ASM = $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) \
			  $(wildcard arch/i686/*.asm) \
			  $(wildcard *.s) \
			  $(wildcard */*.s) \
			  $(wildcard */*/*.s) \
			  $(wildcard */*/*/*.s) \
			  $(wildcard arch/i686/*.s)


OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

.PHONY: all kernel clean always

all: kernel

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "--> Created:  kernel.bin"

$(BUILD_DIR)/kernel/c/%.obj: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	@$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

arch/i686/isrs_gen.c arch/i686/isrs_gen.inc:
	@$(SOURCE_DIR)/build_tools/generate_isrs.sh $@

clean:
	rm -f $(BUILD_DIR)/kernel.bin