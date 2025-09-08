SHELL := /bin/bash
SEED = 42
N = 10000000

all: functionality write_read write_only restore

functionality:
	gcc -Werror -Wpedantic -Wall -Wextra functionality_test.c -o ftest.out
	@echo Functionality Test
	./ftest.out $(SEED) $(N)
	@echo

write_read:
	gcc -Werror -Wpedantic -Wall -Wextra write_read_test.c -o ptestcheck.out -DCHECKPOINT=1
	@echo Test with checkpoint enabled 100% Writes
	time ./ptestcheck.out $(SEED) $(N) 1.0
	@echo
	@echo Test with checkpoint enabled 75% Writes
	time ./ptestcheck.out $(SEED) $(N) 0.75
	@echo
	@echo Test with checkpoint enabled 50% Writes
	time ./ptestcheck.out $(SEED) $(N) 0.5
	@echo
	@echo Test with checkpoint enabled 25% Writes
	time ./ptestcheck.out $(SEED) $(N) 0.25
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra write_read_test.c -o ptest.out -DCHECKPOINT=0
	@echo Test with checkpoint disabled 100% Writes
	time ./ptest.out $(SEED) $(N) 1.0
	@echo
	@echo Test with checkpoint disabled 75% Writes
	time ./ptest.out $(SEED) $(N) 0.75
	@echo
	@echo Test with checkpoint disabled 50% Writes
	time ./ptest.out $(SEED) $(N) 0.50
	@echo
	@echo Test with checkpoint disabled 25% Writes
	time ./ptest.out $(SEED) $(N) 0.25
	@echo

write_only:
	gcc -Werror -Wpedantic -Wall -Wextra only_write_test.c -o only_write.out -DCHECKPOINT=1
	@echo Test with checkpoint enabled only writes
	time ./only_write.out $(N)
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra only_write_test.c -o only_write.out -DCHECKPOINT=0
	@echo Test with checkpoint disabled only writes
	time ./only_write.out $(N)
	@echo

restore:
	gcc -Werror -Wpedantic -Wall -Wextra -mavx restore_test.c -o rest.out -DSIMPLE
	@echo Restore Test SIMPLE	
	./rest.out $(SEED) $(N)
	@echo
 
	gcc -Werror -Wpedantic -Wall -Wextra -mavx restore_test.c -o rest.out -DSIMPLE2
	@echo Restore Test SIMPLE 2
	./rest.out $(SEED) $(N)
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra -mavx restore_test.c -o rest.out -DSIMPLE3
	@echo Restore Test SIMPLE 3
	./rest.out $(SEED) $(N)
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra -mavx restore_test.c -o rest.out -DAVX8
	@echo Restore Test AVX8	
	./rest.out $(SEED) $(N)
	@echo

	gcc -Werror -Wpedantic -Wall -Wextra -mavx restore_test.c -o rest.out -DAVX16
	@echo Restore Test AVX16	
	./rest.out $(SEED) $(N)
	@echo
	
clean:
	rm -f *.out