TARGET = sine-demo
OBJS =   main.o sine_table.o fc.o fcplay.o lamepaula.o

INCDIR   =
CFLAGS   = -G0 -Wall -O2 -fno-strict-aliasing
CXXFLAGS = $(CFLAGS) -Wno-deprecated  -fno-rtti
ASFLAGS  = $(CFLAGS)
LIBS 	 =  -lstdc++ -lm  -lpspaudiolib -lpspaudio -lpspgum -lpspgu

LIBDIR =
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Sine04

PSPSDK=/usr/local/pspdev/psp/sdk
include $(PSPSDK)/lib/build.mak

run: $(TARGET).elf
	$$(PSP) $(TARGET).elf
