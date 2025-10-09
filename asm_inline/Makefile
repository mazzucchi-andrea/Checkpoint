SHELL := /bin/bash
SIZE = 0x100000UL
WRITES = 300000
READS = 700000

all: 

	gcc full_test_suite.c -o test.out -DSIZE=$(SIZE) -DMOD=64
	@echo Full Test Suite 64 bit checkpoint
	./test.out $(WRITES) $(READS)
	@echo

	gcc full_test_suite.c -o test.out -DSIZE=$(SIZE) -DMOD=128
	@echo Full Test Suite 128 bit checkpooint
	./test.out $(WRITES) $(READS)
	@echo
	
clean:
	rm -f *.out