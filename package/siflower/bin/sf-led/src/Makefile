SRC=main.c led.c
EXEC=sf_led

SRB=netdetect.c led.c
EXEB=net_detect

all: $(EXEC) $(EXEB)

sf_led: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(INCLUDES) $(LIBS)

net_detect: $(SRB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRB) $(INCLUDES) $(LIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
	-rm -f $(EXEB) *.elf *.gdb *.o
