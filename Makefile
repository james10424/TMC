all: TMC

FLAGS = -g -Wall -Werror
LIBS = -lncurses -lpthread

%.o: %.c
	gcc $(FLAGS) -c $< -o $@

TMC: TMC.o
	gcc $(FLAGS) $(LIBS) $< -o $@

clean:
	rm *.o TMC