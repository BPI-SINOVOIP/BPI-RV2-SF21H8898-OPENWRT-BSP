include $(TOPDIR)/.config
EXEC = memdump
SRC = memdump.c

all: $(EXEC)

memdump:$(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)
clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
