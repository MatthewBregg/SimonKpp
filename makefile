# Makefile for AVR C++ projects
# From: https://gist.github.com/rynr/72734da4b8c7b962aa65

# ----- Update the settings of your project here -----

# Stack arduino compilation commands
# https://pastebin.com/kwbRLqsE

# See also
# https://blog.podkalicki.com/how-to-compile-and-burn-the-code-to-avr-chip-on-linuxmacosxwindows/

# To get assembly output for debug,
# avr-g++ -Wa,-adhln -ggdb  -O3 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -std=c++17 -DF_OSC=16000000UL  -mmcu=atmega8  -fno-exceptions  *.cc | less



# Hardware
MCU     = atmega8 # see `make show-mcu`
OSC     = 16000000UL # Cpu frequency
PROJECT = SimonKpp

# ----- These configurations are quite likely not to be changed -----

# Binaries
GCC     = avr-gcc
G++     = avr-g++
RM      = rm -f
AVRDUDE = avrdude
AVROBJCOPY = avr-objcopy

# Files
EXT_C   = c
EXT_C++ = cc
EXT_ASM = asm

# ----- No changes should be necessary below this line -----

OBJECTS = \
	$(patsubst %.$(EXT_C),%.o,$(wildcard *.$(EXT_C))) \
	$(patsubst %.$(EXT_C++),%.o,$(wildcard *.$(EXT_C++))) \
	$(patsubst %.$(EXT_ASM),%.o,$(wildcard *.$(EXT_ASM)))

# TODO explain these flags, make them configurable
CFLAGS = $(INC)
CFLAGS += -O3
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
# https://stackoverflow.com/questions/15606993/g-optimization-flags-fuse-linker-plugin-vs-fwhole-program
CFLAGS += -flto
CFLAGS += -fuse-linker-plugin
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -DF_OSC=$(OSC)
CFLAGS += -mmcu=$(MCU)

C++FLAGS = $(INC)
C++FLAGS += -O3
# LTO is important for code size!!
# https://eklitzke.org/how-gcc-lto-works
# https://wiki.debian.org/LTO
# https://stackoverflow.com/questions/23736507/is-there-a-reason-why-not-to-use-link-time-optimization-lto
C++FLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -flto
# https://stackoverflow.com/questions/15606993/g-optimization-flags-fuse-linker-plugin-vs-fwhole-program
C++FLAGS += -fuse-linker-plugin
C++FLAGS += -Wall -std=c++17
C++FLAGS += -DF_OSC=$(OSC)
C++FLAGS += -mmcu=$(MCU)
C++FLAGS += -fno-exceptions

ASMFLAGS = $(INC)
ASMFLAGS += -Os
ASMFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
ASMFLAGS += -Wall -Wstrict-prototypes
ASMFLAGS += -DF_OSC=$(OSC)
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -mmcu=$(MCU)

default: $(PROJECT).hex
	echo $(OBJECTS)

flash: $(PROJECT).hex
	$(AVRDUDE) -p $(MCU) -c usbasp -U flash:w:$(PROJECT).hex:i -F -P usb

%.hex: $(PROJECT).elf
	$(AVROBJCOPY) -j .text -j .data -O ihex $(PROJECT).elf $(PROJECT).hex

%.elf: $(OBJECTS)
	$(GCC) $(C++FLAGS) $(OBJECTS) --output $@ $(LDFLAGS)

%.o : %.$(EXT_C)
	$(GCC) $< $(CFLAGS) -c -o $@

%.o : %.$(EXT_C++) *.h
	$(G++) $< $(C++FLAGS) -c -o $@

%.o : %.$(EXT_ASM)
	$(G++) $< $(ASMFLAGS) -c -o $@

clean:
	$(RM) $(PROJECT).elf $(PROJECT).hex $(OBJECTS)

help:
	@echo "usage:"
	@echo "  make <target>"
	@echo ""
	@echo "targets:"
	@echo "  clean     Remove any non-source files"
	@echo "  config    Shows the current configuration"
	@echo "  help      Shows this help"
	@echo "  show-mcu  Show list of all possible MCUs"

config:
	@echo "configuration:"
	@echo ""
	@echo "Binaries for:"
	@echo "  C compiler:   $(GCC)"
	@echo "  C++ compiler: $(G++)"
	@echo "  Programmer:   $(AVRDUDE)"
	@echo "  remove file:  $(RM)"
	@echo ""
	@echo "Hardware settings:"
	@echo "  MCU: $(MCU)"
	@echo "  OSC: $(OSC)"
	@echo ""
	@echo "Defaults:"
	@echo "  C-files:   *.$(EXT_C)"
	@echo "  C++-files: *.$(EXT_C++)"
	@echo "  ASM-files: *.$(EXT_ASM)"

show-mcu:
	$(G++) --help=target

