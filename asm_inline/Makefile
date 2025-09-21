SHELL := /bin/bash
SIZE = 0x200000
WRITES = 10000000
READS = 0

all: 
	gcc -mavx full_test_suite.c -o test.out -DSIMPLE
	@echo Full Test Suite with SIMPLE restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DSIMPLE2
	@echo Full Test Suite with SIMPLE2 restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DSIMPLE3
	@echo Full Test Suite with SIMPLE3 restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DAVX8
	@echo Full Test Suite with AVX8 restore
	./test.out $(WRITES) $(READS)
	@echo

	gcc -mavx full_test_suite.c -o test.out -DAVX16
	@echo Full Test Suite with AVX16 restore
	./test.out $(WRITES) $(READS)
	@echo
	
clean:
	rm -f *.out