EXEC = modbus-log
OBJS = modbus-log.o
CC=gcc

#-I/usr/include/modbus -lmodbus
LDLIBS += -lcurl
LDLIBS += -lmodbus

CFLAGS = -Wall -g -O2 -I/usr/include/modbus


all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	

restart: all
	systemctl restart $(EXEC)
clean:
	-rm -f  *.elf *.gdb *.o




