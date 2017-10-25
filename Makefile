default: main.c
	cc -std=c99 -Wall main.c mpc.c -ledit -o Lisp
clean:
	rm -f ./List
