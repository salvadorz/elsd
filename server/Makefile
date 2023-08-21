.DEFAULT_GOAL := all
# Query the default target.
ifeq ($(.DEFAULT_GOAL),)
  $(warning no default target is set)
endif

#CROSS_COMPILE = aarch64-none-linux-gnu-
ifdef CROSS_COMPILE
	CC ?= $(CROSS_COMPILE)gcc
	CFLAGS ?= -g -Wall -Wextra
endif

LIBS ?= -lpthread -lrt
LDFLAGS ?= $(LIBS)
INCLUDES = -I../aesd-char-driver
SRC := aesdsocket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	$(RM) *.o $(TARGET) *.elf *.map


depend:

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<
