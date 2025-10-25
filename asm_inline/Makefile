SHELL := /bin/bash
SIZE = 0x100000UL
WRITES = 950
READS = 50
MOD = 64

all: 

	gcc full_test_suite.c -mavx -mavx2 -mavx512f -o test.out -DSIZE=$(SIZE) -DMOD=64
	@echo Full Test Suite 64 bit checkpoint
	./test.out $(WRITES) $(READS)
	@echo

	gcc full_test_suite.c -mavx -mavx2 -mavx512f -o test.out -DSIZE=$(SIZE) -DMOD=128
	@echo Full Test Suite 128 bit checkpooint
	./test.out $(WRITES) $(READS)
	@echo

	gcc full_test_suite.c -mavx -mavx2 -mavx512f -o test.out -DSIZE=$(SIZE) -DMOD=256
	@echo Full Test Suite 256 bit checkpooint
	./test.out $(WRITES) $(READS)
	@echo

	gcc full_test_suite.c -mavx -mavx2 -mavx512f -o test.out -DSIZE=$(SIZE) -DMOD=512
	@echo Full Test Suite 512 bit checkpooint
	./test.out $(WRITES) $(READS)
	@echo

tests:
	gcc full_test_suite.c -mavx -mavx2 -mavx512f -o test.out -DSIZE=$(SIZE) -DMOD=$(MOD)
	./test.out $(WRITES) $(READS)
clean:
	rm -f *.out