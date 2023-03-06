.DEFAULT_GOAL := all
# Query the default target.
ifeq ($(.DEFAULT_GOAL),)
  $(warning no default target is set)
endif

#CROSS_COMPILE = aarch64-none-linux-gnu-
ifdef CROSS_COMPILE
	CC := $(CROSS_COMPILE)gcc
	#CFLAGS = -g -Wall
endif

SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
