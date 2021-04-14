makemain:
	gcc main.c -lpthread

rst: clean run

run: all
	prog

all: queue.o test.o
	gcc -o prog queue.o test.o

queue.o:
	gcc -c queue.c

test.o:
	gcc -c test.c

clean:
	rm *.o
	rm prog