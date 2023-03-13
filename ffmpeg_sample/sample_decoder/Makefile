PKG_CONFIG_PATH = $(shell printenv PWD)/../install/lib/pkgconfig:$(shell printenv PKG_CONFIG_PATH)
$(info LD_LIBRARY_PATH: $(LD_LIBRARY_PATH))
export PKG_CONFIG_PATH

CC = gcc
XX = g++
CFLAGS = -Wall -O -g
LDFLAGS =

TARGET = test


CFLAGS += `pkg-config --cflags libavcodec libavutil`
LDFLAGS += `pkg-config --libs libavcodec libavutil`
LDFLAGS += -lx264

$(info CFLAGS: $(CFLAGS))
$(info LDFLAGS: $(LDFLAGS))

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o:%.cpp
	$(xx) $(CFLAGS) -c $< -o $@

SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

$(TARGET):$(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	chmod a+x $(TARGET)

clean:
	rm $(OBJS) $(TARGET)
