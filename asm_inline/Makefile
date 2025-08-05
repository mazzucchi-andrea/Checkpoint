SHELL := /bin/bash
SEED = 42
N = 10000000

all:
	gcc -Werror -Wpedantic -Wall -Wextra functionality_test.c -o ftest.out
	@echo Functionality Test
	./ftest.out $(SEED) $(N)
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra performance_test.c -o ptestcheck.out -DCHECKPOINT=1
	@echo Test with checkpint enabled 100% Writes
	time ./ptestcheck.out $(SEED) $(N) 1.0
	@echo
	@echo Test with checkpint enabled 75% Writes
	time ./ptestcheck.out $(SEED) $(N) 0.75
	@echo
	@echo Test with checkpint enabled 50% Writes
	time ./ptestcheck.out $(SEED) $(N) 0.5
	@echo
	@echo Test with checkpint enabled 25% Writes
	time ./ptestcheck.out $(SEED) $(N) 0.25
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra performance_test.c -o ptest.out -DCHECKPOINT=0
	@echo Test with checkpint disabled 100% Writes
	time ./ptest.out $(SEED) $(N) 1.0
	@echo
	@echo Test with checkpint disabled 75% Writes
	time ./ptest.out $(SEED) $(N) 0.75
	@echo
	@echo Test with checkpint disabled 50% Writes
	time ./ptest.out $(SEED) $(N) 0.50
	@echo
	@echo Test with checkpint disabled 25% Writes
	time ./ptest.out $(SEED) $(N) 0.25
	@echo
	
clean:
	rm -f *.out