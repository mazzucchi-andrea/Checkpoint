SHELL := /bin/bash
SIZE = 0x400000UL
WRITES = 10000000
READS = 0

all: 
	gcc -mavx full_test_suite.c -o test.out -DSIMPLE -DSIZE=$(SIZE)
	@echo Full Test Suite with SIMPLE restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DSIMPLE2 -DSIZE=$(SIZE)
	@echo Full Test Suite with SIMPLE2 restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DSIMPLE3 -DSIZE=$(SIZE)
	@echo Full Test Suite with SIMPLE3 restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DAVX8 -DSIZE=$(SIZE)
	@echo Full Test Suite with AVX8 restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DAVX16 -DSIZE=$(SIZE)
	@echo Full Test Suite with AVX16 restore
	./test.out $(WRITES) $(READS)
	@echo
	
clean:
	rm -f *.out