CFLAGS = -Wall

#LIBS = -lnvpair

libnictag.so: nictag.o
	$(CC) -o $@ $^ -shared

nictag.o: nictag.c libnictag.h
	$(CC) -o $@ -c $< -fpic

.PHONY: clean
clean:
	rm -f *.o *.so
