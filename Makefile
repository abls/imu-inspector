CC = gcc
LIBS = -lhidapi-libusb -lncurses
TARGET = inspector

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $^ -o $@ $(LIBS)

clean:
	$(RM) $(TARGET)
