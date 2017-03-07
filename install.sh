#!/bin/bash
cat >> ~/PA3/Makefile <<'EOF'

TESTDIR = $(HOME)/PA3/ucsd-cs120-wi16-pa3-tester

semtest: 	$(TESTDIR)/semtest.c aux.h umix.h mykernel3.o
		gcc -std=c99 -o semtest $(TESTDIR)/semtest.c mykernel3.o 
 	
EOF
