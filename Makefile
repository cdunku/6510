.PHONY: all
.PHONY: clean

CC := gcc
CFLAGS := -O3 -ggdb -pthread -pedantic
LDFLAGS := --print-memory-usage
BIN = 6510

# -fsanitize=address,undefined 

SRCDIR = $(wildcard *.c) 
OBJS = $(SRCDIR:.c=.o)

all: $(OBJS)
	$(CC) -o $(BIN) $(OBJS) 

%: %.c 
	$(CC) $(CFLAGS)  "$<" -c "$@"

clean:
	rm -rvf $(OBJS) $(BIN) *.gch
