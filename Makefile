lconv1: lconv1.c
	gcc -o lconv1 lconv1.c

lconv2: lconv2.c
	gcc -o lconv2 lconv2.c -lm

lconv3: lconv3.c
	gcc -o lconv3 lconv3.c -lm

lconv4: lconv4.c vector3.c vector3.h
	gcc -o lconv4 lconv4.c vector3.c -lm
	
lconv: lconv.c
	gcc -o lconv lconv.c

test: bufTest.c vector3.c vector3.h
	gcc -g -o bufTest bufTest.c vector3.c

all: lconv1 lconv4

clean:
	# rm lconv?
	rm lconv?.exe