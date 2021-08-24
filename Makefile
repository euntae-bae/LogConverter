lconv1: lconv1.c
	gcc -o lconv1 lconv1.c

lconv4: lconv4.c vector3.c vector3.h
	gcc -o lconv4 lconv4.c vector3.c -lm

test: bufTest.c vector3.c vector3.h
	gcc -g -o bufTest bufTest.c vector3.c

stepcnt: stepcnt.c
	gcc -o stepcnt stepcnt.c

all: lconv1 lconv4 stepcnt

clean:
	# rm lconv?
	rm lconv?.exe
	rm stepcnt.exe