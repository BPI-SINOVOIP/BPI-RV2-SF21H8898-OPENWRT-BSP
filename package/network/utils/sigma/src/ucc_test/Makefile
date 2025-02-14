ucc_OBJS = ucc.o
CFLAGS += -Wall -g 
all: ucc
%.o:%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

ucc: $(ucc_OBJS)
	$(CC) -o $@ $(ucc_OBJS)

clean:
	rm -f rbcfg *.o
	rm -f rbcfg *.so
