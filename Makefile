CFLAGS = -g -Wall -Werror
OBJS = pigc.o gpio.o uinput.o

all: pigc

pigc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): gpio.h uinput.h

clean:
	rm -f pigc $(OBJS)

install:
	cp pigc /usr/local/bin
	chmod 755 /usr/local/bin/pigc
