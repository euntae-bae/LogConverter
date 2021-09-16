lconv1: lconv1.c
	gcc -o lconv1 lconv1.c

lconv5: lconv5.c vector3.c vector3.h
	gcc -o lconv5 lconv5.c vector3.c -lm

lconv6: lconv6.c
	gcc -o lconv6 lconv6.c -lm

test: bufTest.c vector3.c vector3.h
	gcc -g -o bufTest bufTest.c vector3.c

stepcnt: stepcnt.c
	gcc -D _SVM_MODE -W -Wall -o stepcnt stepcnt.c

stepcnt-dbg: stepcnt.c
	gcc -D _DEBUG -D _SVM_MODE -g -W -Wall -o stepcnt-dbg stepcnt.c

all: lconv1 lconv5 lconv6 stepcnt

debug: stepcnt-dbg

clean:
	rm lconv?
	rm stepcnt
	rm stepcnt-dbg
