CC=gcc
CFLAGS=-Wall -g
LD=gcc
LDFLAGS=-g
LDLIBS = -lz

LIB_UTIL = zutil.o crc.o lab_png.o
OBJS1 = pnginfo.o lab_png.o crc.o
OBJS2 = findpng.o lab_png.o crc.o
OBJS3 = catpng.o lab_png.o zutil.o crc.o

all: pnginfo findpng catpng

pnginfo: $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

findpng: $(OBJS2)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

catpng: $(OBJS3)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

%.d: %.c
	gcc -MM -FF $@ $<

.PHONY: clean
clean:
	rm -f *.f *.o *.out *.d all.png pnginfo findpng catpng
