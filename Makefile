lconv1: lconv1.c
	gcc -o lconv1 lconv1.c

lconv2: lconv2.c
	gcc -o lconv2 lconv2.c -lm

lconv3: lconv3.c
	gcc -o lconv3 lconv3.c -lm
	
lconv: lconv.c
	gcc -o lconv lconv.c

all: lconv1 lconv3 lconv

clean:
	rm *.exe