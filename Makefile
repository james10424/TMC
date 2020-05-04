all: TMC

%.o: %.c
	gcc -g -c -Werror -Wall $<
TMC: TMC.o
	gcc -g -lncurses -lpthread -Werror -Wall TMC.c -o TMC
clean:
	rm *.o TMC