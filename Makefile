prefix = /usr/usr/local

BIN = pw-synth
TARGET = src/$(BIN)
SRCS = $(wildcard src/*.c)

CC = gcc

CFLAGS := `pkg-config --cflags libpipewire-0.3 sdl2`

LDFLAGS := `pkg-config --libs libpipewire-0.3 sdl2`
LDFLAGS += -lm
LDFLAGS += -Wl,-rpath=/usr/lib/lib/x86_64-linux-gnu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET)

distclean: clean

install: $(TARGET)
	install -D $(TARGET) $(DESTDIR)$(prefix)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)$(prefix)/bin/$(BIN)

.PHONY: all clean distclean install uninstall
