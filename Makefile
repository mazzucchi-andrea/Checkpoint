all:
	gcc -Werror -Wpedantic -Wall -Wextra main.c

clean:
	rm -f a.out