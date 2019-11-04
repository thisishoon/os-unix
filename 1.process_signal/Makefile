
TARGETS := procman task

PINIT_OBJS := procman.o

TASK_OBJS := task.o

OBJS := $(PINIT_OBJS) $(TASK_OBJS)

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
	./procman config.txt
#	./procman config1.txt 2> result1.txt

procman: $(PINIT_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

task: $(TASK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
