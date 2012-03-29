all: generate run

generate: generate.c
	gcc -g -o generate generate.c

pqsort.o: pqsort.c
	gcc -g -c -lm -lpthread pqsort.c

linear.o: linear.c linear.h
	gcc -g -c -o linear.o -lpthread linear.c

run: driver.c pqsort.o linear.o
	gcc -g -o run driver.c pqsort.o linear.o -lm -lpthread

clean:
	rm generate pqsort.o linear.o run

logbar:
	gcc -g -o logbarrexe logbarrier.c -lm -lpthread
