
TARGETS := multisched

SCHED_OBJS := multisched.o

OBJS := $(SCHED_OBJS)

CC := gcc

CFLAGS += -D_REENTRANT -D_LIBC_REENTRANT -D_THREAD_SAFE
CFLAGS += -Wall
CFLAGS += -Wunused
CFLAGS += -Wshadow
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wdisabled-optimization
CFLAGS += -Wpointer-arith
CFLAGS += -Wredundant-decls
CFLAGS += -g -O2

LDFLAGS +=

%.o: %.c
	$(CC) -o $*.o $< -c $(CFLAGS)

.PHONY: all clean test

all: $(TARGETS)

clean:
	-rm -f $(TARGETS) $(OBJS) *~ *.bak core*

test: $(TARGETS)
	./multisched data1.txt 

multisched: $(SCHED_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
