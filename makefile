all: socklib.o my_web_server.out

socklib.o: socklib.c socklib.h
	cc -c socklib.c -o socklib.o -g

my_web_server.out: socklib.o my_web_server.c web_util.h web_util.c child_processor.c child_processor.h
	cc my_web_server.c web_util.c child_processor.c socklib.o -o my_web_server.out -g

clean:
	rm my_web_server.log my_web_server.out socklib.o