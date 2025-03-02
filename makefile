all: socklib.o tester.out my_web_server.out

socklib.o: socklib.c socklib.h
	cc -c socklib.c -o socklib.o -g

my_web_server.out: socklib.o my_web_server.c
	cc my_web_server.c socklib.o -o my_web_server.out -g

tester.out: tester.c socklib.o
	cc tester.c socklib.o -o tester.out -g