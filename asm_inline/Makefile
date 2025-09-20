SHELL := /bin/bash
WRITES = 10000000
READS = 0

all: restore
	gcc full_test_suite.c -o test.out
	@echo Full Test Suite 
	./test.out $(WRITES) $(READS)

restore:
	gcc -mavx restore_test.c -o rest.out -DSIMPLE
	@echo Restore Test SIMPLE	
	./rest.out $(WRITES)
	@echo
 
	gcc -mavx restore_test.c -o rest.out -DSIMPLE2
	@echo Restore Test SIMPLE 2
	./rest.out $(WRITES)
	@echo

	gcc -mavx restore_test.c -o rest.out -DSIMPLE3
	@echo Restore Test SIMPLE 3
	./rest.out $(WRITES)
	@echo

	gcc -mavx restore_test.c -o rest.out -DAVX8
	@echo Restore Test AVX8	
	./rest.out $(WRITES)
	@echo

	gcc -mavx restore_test.c -o rest.out -DAVX16
	@echo Restore Test AVX16	
	./rest.out $(WRITES)
	@echo
	
clean:
	rm -f *.out