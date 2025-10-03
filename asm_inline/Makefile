SHELL := /bin/bash
SIZE = 0x100000UL
WRITES = 10000
READS = 0

all: 64bit 128bit

64bit: 
	gcc -mavx full_test_suite_64bit.c -o test.out -DSIMPLE -DSIZE=$(SIZE)
	@echo Full Test Suite with SIMPLE restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite_64bit.c -o test.out -DAVX -DSIZE=$(SIZE)
	@echo Full Test Suite with AVX restore
	./test.out $(WRITES) $(READS)
	@echo

128bit: 
	gcc full_test_suite_128bit.c -o test.out -DSIZE=$(SIZE)
	@echo Full Test Suite with SIMPLE restore
	./test.out $(WRITES) $(READS)
	@echo
	
clean:
	rm -f *.out