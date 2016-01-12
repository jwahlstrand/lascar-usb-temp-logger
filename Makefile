PROGRAM = temp_logger
PROGRAM_FILES = temp_logger.c

CC = gcc

CFLAGS	+= -g $(shell pkg-config --cflags libhid libsoup-2.4)
LDFLAGS	+= -g
LIBS 	+= $(shell pkg-config --libs libhid libsoup-2.4)

SOURCES = $(wildcard *.c)

OBJS = $(SOURCES:.c=.o)

ALL: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY clean:
	@rm -rf $(PROGRAM)
	@rm -rf $(OBJS)

