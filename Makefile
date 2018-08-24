
INCPATH = -I /usr/local/include/libusb-1.0/
LIBSPATH = -L /usr/local/lib/
LIBS = $(LIBSPATH) -lusb-1.0

all:
	$(CC) hub_test_mode.c  -Werror -o hub_test_mode $(INCPATH) $(LIBS)

clean:
	rm -rf hub_test_mode
