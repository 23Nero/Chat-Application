.PHONY: all clean run

all: clean
	gcc main.c -o main -lpthread
run1: 
	./main 2000
run2: 
	./main 3000
run3: 
	./main 4000
clean:
	rm -rf main
