CC = gcc
CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-pic -nostdlib -I. -I../src/kernel
LDFLAGS = -m32 -Wl,-Ttext-segment,0x1000000 -nostdlib -Wl,-e,_start -no-pie

TARGET = hello.elf
SRC = hello.c libmm8.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean