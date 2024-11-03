all : clean compile run

compile : 
	@gcc -g -o hw3 hw3.c -lpthread

run : 
	./hw3 
clean :
	@rm -f *.o
	@rm -f hw3

.PHONY: all compile run clean
	
	
