# GNU make and a POSIX shell (e.g. Linux, macOS, MSYS2 or Git Bash).
# Firmware defaults live in src/config.h; add -D options through CPPFLAGS.
# Run make clean when changing compiler or command-line flags.

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

MCU = attiny202
TARGET = build/orbiter
AVR_PREFIX ?= avr-

CC = $(AVR_PREFIX)gcc
OBJCOPY = $(AVR_PREFIX)objcopy
OBJDUMP = $(AVR_PREFIX)objdump
SIZE = $(AVR_PREFIX)size

AVRDUDE = avrdude
AVRDUDE_FLAGS = -A -v
PROGRAMMER = jtag2updi
PORT ?=

# Keep the source include path when CPPFLAGS is supplied on the command line.
override CPPFLAGS += -Isrc
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -fshort-enums \
         -ffunction-sections -fdata-sections -fstack-usage -flto
ASFLAGS = -g3
LDFLAGS = -Os -g3 -flto -fstack-usage -save-temps=obj \
          -Wl,--gc-sections,-Map=$(TARGET).map

C_SOURCES = src/main.c src/app.c src/recovery.c src/leds.c \
            src/board.c
ASM_SOURCES = src/ws2812.S
OBJECTS = $(C_SOURCES:%.c=build/%.o) $(ASM_SOURCES:%.S=build/%.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all build size clean flash help

all:
	$(MAKE) clean
	$(MAKE) build

build: $(TARGET).hex $(TARGET).lst size

$(OBJECTS): Makefile

build/%.o: %.c
	@mkdir -p "$(@D)"
	"$(CC)" -mmcu=$(MCU) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c "$<" -o "$@"

build/%.o: %.S
	@mkdir -p "$(@D)"
	"$(CC)" -mmcu=$(MCU) $(CPPFLAGS) $(ASFLAGS) -MMD -MP -c "$<" -o "$@"

$(TARGET).elf: $(OBJECTS)
	"$(CC)" -mmcu=$(MCU) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o "$@"

# Retain tinyAVR flash-mapped constants; exclude EEPROM, fuses and lock bits.
$(TARGET).hex: $(TARGET).elf
	"$(OBJCOPY)" -j .text -j .rodata -j .data -O ihex "$<" "$@"

$(TARGET).lst: $(TARGET).elf
	"$(OBJDUMP)" -d -z -S "$<" > "$@"

size: $(TARGET).elf
	"$(SIZE)" --format=avr --mcu=$(MCU) "$<"

# Explicitly invoked only; writes and verifies application flash.
# No EEPROM, fuse or lock-write commands.
flash: all
	@test -n "$(PORT)" || (echo "Specify PORT, e.g. make flash PORT=COM8"; exit 1)
	"$(AVRDUDE)" $(AVRDUDE_FLAGS) -c $(PROGRAMMER) -P "$(PORT)" -p t202 \
        -U "flash:w:$(TARGET).hex:i"

clean:
	$(RM) -r build

-include $(DEPS)

help:
	@echo "Orbiter 2 Smart Sensor Firmware"
	@echo ""
	@echo "  make                 Build firmware"
	@echo "  make size            Show memory usage"
	@echo "  make flash PORT=COM8 Flash using jtag2updi"
