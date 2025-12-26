TARGET_ASMFLAGS += -f elf64 -Iarch/x86_64/

# Compiler Flags
# -mno-red-zone:  Disable the "red zone" (128 bytes below stack pointer) because interrupts can clobber it.
# -mcmodel=kernel: Use the kernel code model (absolute addressing in upper 2GB or negative 2GB).
# -ffreestanding: No standard library.
TARGET_CFLAGS = -ffreestanding -m64 -g -c -I. -mno-red-zone -mno-mmx -mcmodel=kernel -Wno-attributes #src/cjson
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -z max-page-size=0x1000 -nostdlib

HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SOURCES_C = $(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c)

SOURCES_C_FILTERED = $(filter-out arch/i686/%, $(SOURCES_C)) arch/x86_64/isrs_gen.c

OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C_FILTERED))

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \
			  arch/x86_64/isrs_gen.inc

SOURCES_ASM = $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) \
			  $(wildcard *.s) \
			  $(wildcard */*.s) \
			  $(wildcard */*/*.s) \
			  $(wildcard */*/*/*.s)

SOURCES_ASM_FILTERED = $(filter-out arch/i686/%, $(SOURCES_ASM))

OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM_FILTERED))

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

arch/x86_64/isrs_gen.c arch/x86_64/isrs_gen.inc: $(SOURCE_DIR)/build_scripts/generate_isrs.sh
	@$(SOURCE_DIR)/build_scripts/generate_isrs.sh $@

clean:
	rm -f $(BUILD_DIR)/kernel.bin