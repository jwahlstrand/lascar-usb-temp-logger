PROGRAM = th_logger
PROGRAM_FILES = th_logger.c

CC = gcc

CFLAGS	+= -g $(shell pkg-config --cflags hidapi-hidraw libsoup-2.4)
LDFLAGS	+= -g
LIBS 	+= $(shell pkg-config --libs hidapi-hidraw libsoup-2.4)

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

