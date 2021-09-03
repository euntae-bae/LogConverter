lconv1: lconv1.c
	gcc -o lconv1 lconv1.c

lconv5: lconv5.c vector3.c vector3.h
	gcc -o lconv5 lconv5.c vector3.c -lm

test: bufTest.c vector3.c vector3.h
	gcc -g -o bufTest bufTest.c vector3.c

stepcnt: stepcnt.c
	gcc -o stepcnt stepcnt.c

stepcnt-debug: stepcnt.c
	gcc -g -W -Wall -o stepcnt-dbg stepcnt.c

all: lconv1 lconv5 stepcnt

clean:
	rm lconv?
	rm stepcnt
	rm stepcnt-dbg
