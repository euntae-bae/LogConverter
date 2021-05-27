lconv1: lconv1.c
	gcc -o lconv1 lconv1.c

lconv2: lconv2.c
	gcc -o lconv2 lconv2.c
	
lconv: lconv.c
	gcc -o lconv lconv.c

all: lconv1 lconv2 lconv

clean:
	rm *.o *.exe
