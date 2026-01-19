SIZE=0x100000
WRITES=950
READS=50
CF=0

all:
	gcc -c simple_ckpt_test.c -pie -fPIE -fstack-protector-all -funroll-loops -DSIZE=$(SIZE) -DWRITES=$(WRITES) -DREADS=$(READS) -DCF=$(CF)
	gcc -c ckpt.c -O3 -mavx -mavx2 -mavx512f -pie -fPIE -fstack-protector-all -DSIZE=$(SIZE)
	gcc -O3 -pie -fPIE -fstack-protector-all simple_ckpt_test.o ckpt.o

clean:
	find . -name "*.o" -type f -delete ; rm -f *.out