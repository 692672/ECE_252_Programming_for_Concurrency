# Makefile, ECE252  
# Yiqing Huang, 2018/11/02

CC = gcc 
CFLAGS = -Wall -std=c99 -g # "curl-config --cflags" output is empty  
LD = gcc
LDFLAGS = -std=c99 -g 
LDLIBS = -lz -lcurl # "curl-config --libs" output 


SRCS   = paster2.c main_write_callback.c zutil.c crc.c catpng.c shm_stack.c
OBJS1  = paster2.o zutil.o crc.o catpng.o lab_png.o main_write_callback.o shm_stack.o
OBJS2  = main_write_callback.o
TARGETS= paster2

all: ${TARGETS}

paster2: $(OBJS1) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) -pthread

main_write_callback.out: $(OBJS2)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -rf *~ *.d *.o $(TARGETS) *.png images
	./clean_ipcs.sh
