
CFLAGS=-Wall -Wextra -I/opt/local/include/libbson-1.0
LIBS=-L/opt/local/lib  -lbson-static-1.0

simple: simple.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

run: simple
	./simple

