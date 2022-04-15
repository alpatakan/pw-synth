TARGET := pw-synth
SRCS := sin-synth.c
OBJS := $(SRCS:.c=.o)

CC := gcc

CFLAGS := `pkg-config --cflags libpipewire-0.3 sdl2`

LDFLAGS := `pkg-config --libs libpipewire-0.3 sdl2`
LDFLAGS += -lm
LDFLAGS += -Wl,-rpath=/usr/lib/lib/x86_64-linux-gnu

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET)
