TARGET = netlink_pslist

$(TARGET): pslist-client.o
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $?

pslist-client.o: pslist-client.c
	$(CC) $(CCFLAGS) -Wall -pedantic -std=c99 -c $< -o $@

.PHONY: clean
clean:
	-rm -f core $(TARGET)
	-rm -f *.o

