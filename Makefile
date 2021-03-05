CC=gcc
CFLAGS=-Wall -O3
TARGET=screenshot
LFLAGS=$(shell pkg-config --cflags --libs gtk+-3.0) -lX11 -lXext -lm

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $(LFLAGS) main.c -o $(TARGET) 

clean:
	rm -f $(TARGET)
