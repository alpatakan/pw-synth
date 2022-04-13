TARGET=pw-synth
SRCS=sin-synth.c
OBJS=$(SRCS:.c=.o)

CC=gcc
CFLAGS=-I/usr/lib/include/spa-0.2 -I/usr/lib/include/pipewire-0.3
LDFLAGS=-L/usr/lib/lib/x86_64-linux-gnu -lpipewire-0.3 -lm -Wl,-rpath=/usr/lib/lib/x86_64-linux-gnu

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET)
