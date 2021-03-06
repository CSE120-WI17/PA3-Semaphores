# Makefile to compile Umix Programming Assignment 3 (pa3) [updated: 1/11/10]

LIBDIR = $(UMIXPUBDIR)/lib

CC 	= cc 
FLAGS 	= -g -L$(LIBDIR) -lumix3

PA3 =	pa3a pa3b pa3c pa3d philosophers testcases

pa3:	$(PA3)

pa3a:	pa3a.c aux.h umix.h mykernel3.o
	$(CC) $(FLAGS) -o pa3a pa3a.c mykernel3.o

pa3b:	pa3b.c aux.h umix.h mykernel3.o
	$(CC) $(FLAGS) -o pa3b pa3b.c mykernel3.o

pa3c:	pa3c.c aux.h umix.h mykernel3.o
	$(CC) $(FLAGS) -o pa3c pa3c.c mykernel3.o

pa3d:	pa3d.c aux.h umix.h mykernel3.o
	$(CC) $(FLAGS) -o pa3d pa3d.c mykernel3.o

philosophers: philosophers.c aux.h umix.h mykernel3.o
	$(CC) $(FLAGS) -o philosophers philosophers.c mykernel3.o
	
testcases: testcases.c aux.h umix.h mykernel3.o
	$(CC) $(FLAGS) -o testcases testcases.c mykernel3.o
	
mykernel3.o:	mykernel3.c aux.h sys.h mykernel3.h
	$(CC) $(FLAGS) -c mykernel3.c

clean:
	rm -f *.o $(PA3)

TESTDIR = $(HOME)/PA3/ucsd-cs120-wi16-pa3-tester

semtest: 	$(TESTDIR)/semtest.c aux.h umix.h mykernel3.o
		gcc -std=c99 -o semtest $(TESTDIR)/semtest.c mykernel3.o 
 	
