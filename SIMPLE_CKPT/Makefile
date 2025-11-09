SIZE=0x100000UL
WRITES=950
READS=50
CF=0

all:
	gcc simple_ckpt_test.c -funroll-loops -DSIZE=$(SIZE) -DWRITES=$(WRITES) -DREADS=$(READS) -DCF=$(CF)

clean:
	rm -f *.out