# Template makefile that is included by project makefiles
# Based on blinky.mak in third_party/Teensy3x

# Set the following variables in the project makefile:
# PROJECT: name of the project (determines .hex and .bin, etc)
# OBJECTS: list of *.o corresponding to *.c in the project

#########################################################################
# Set variables

CPU = cortex-m4

OBJECTS	+= sysinit.o crt0.o arm_cm4.o

TOOLPATH = /opt/gcc-arm-none-eabi-5_2-2015q4

# Assuming main Makefile is in projects/PROJECT/
TEENSY3X_BASEPATH = ../..

TARGETTYPE = arm-none-eabi

TEENSY3X_INC     = $(TEENSY3X_BASEPATH)/include
GCC_INC          = $(TOOLPATH)/$(TARGETTYPE)/include

VPATH = $(TEENSY3X_BASEPATH)/common

INCDIRS  = -I$(GCC_INC)
INCDIRS += -I$(TEENSY3X_INC)
INCDIRS += -I.

LSCRIPT = $(TEENSY3X_BASEPATH)/common/Teensy31_flash.ld

OPTIMIZATION = 0
DEBUG = -g

LIBDIRS  = -L"$(TOOLPATH)\$(TARGETTYPE)\lib"
LIBS =

GCFLAGS = -Wall -fno-common -mcpu=$(CPU) -mthumb -O$(OPTIMIZATION) $(DEBUG)
GCFLAGS += $(INCDIRS)

ASFLAGS = -mcpu=$(CPU)

LDFLAGS  = -nostdlib -nostartfiles -Map=$(PROJECT).map -T$(LSCRIPT)
LDFLAGS += --cref
LDFLAGS += $(LIBDIRS)
LDFLAGS += $(LIBS)

BINDIR = $(TOOLPATH)/bin
UPLOADDIR = $(TEENSY3X_BASEPATH)/third_party/teensy_loader_cli
UPLOADER = teensy_loader_cli

#########################################################################
#  Tools paths

CC = $(BINDIR)/arm-none-eabi-gcc
AS = $(BINDIR)/arm-none-eabi-as
AR = $(BINDIR)/arm-none-eabi-ar
LD = $(BINDIR)/arm-none-eabi-ld
OBJCOPY = $(BINDIR)/arm-none-eabi-objcopy
SIZE = $(BINDIR)/arm-none-eabi-size
OBJDUMP = $(BINDIR)/arm-none-eabi-objdump

REMOVE = rm -f

UPLOAD = $(UPLOADDIR)/$(UPLOADER)

#########################################################################
# Make targets

all: $(PROJECT).hex $(PROJECT).bin stats dump miniclean

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary -j .text -j .data $(PROJECT).elf $(PROJECT).bin

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -R .stack -O ihex $(PROJECT).elf $(PROJECT).hex

$(PROJECT).elf: $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $(PROJECT).elf

stats: $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

dump: $(PROJECT).elf
	$(OBJDUMP) -h $(PROJECT).elf

$(UPLOAD):
	@echo
	@echo Building the uploader now ...
	make -C $(UPLOADDIR)

.PHONY: upload
upload: all $(UPLOAD)
	@echo
	@echo Plug in the Teensy and press the bootloader button now ...
	$(UPLOAD) -w --mcu=mk20dx256 $(PROJECT).hex
	@echo Upload complete!

miniclean:
	$(REMOVE) *.o
	$(REMOVE) *.lst

clean:
	$(REMOVE) *.o
	$(REMOVE) $(PROJECT).hex
	$(REMOVE) $(PROJECT).elf
	$(REMOVE) $(PROJECT).map
	$(REMOVE) $(PROJECT).bin
	$(REMOVE) *.lst

toolvers:
	$(CC) --version | sed q
	$(AS) --version | sed q
	$(LD) --version | sed q
	$(AR) --version | sed q
	$(OBJCOPY) --version | sed q
	$(SIZE) --version | sed q
	$(OBJDUMP) --version | sed q

.c.o :
	@echo Compiling $<, writing to $@...
	$(CC) $(GCFLAGS) -c $< -o $@ > $(basename $@).lst

.cpp.o :
	@echo Compiling $<, writing to $@...
	$(CC) $(GCFLAGS) -c $<

.s.o :
	@echo Assembling $<, writing to $@...
	$(AS) $(ASFLAGS) -o $@ $<  > $(basename $@).lst
