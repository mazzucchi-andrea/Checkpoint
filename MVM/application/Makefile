ALLOCATOR_AREA_SIZE=0x100000
WRITES=950
READS=50
MOD=8
CF=0

all:
	gcc -c ckpt.c -O3 -mavx -mavx2 -mavx512f -pie -fPIE -fstack-protector-all -DALLOCATOR_AREA_SIZE=$(ALLOCATOR_AREA_SIZE) -DMOD=$(MOD)
	gcc -c prog.c -funroll-loops -pie -fPIE -fstack-protector-all -I ./ -I ../include -DALLOCATOR_AREA_SIZE=$(ALLOCATOR_AREA_SIZE) -DWRITES=$(WRITES) -DREADS=$(READS) -DCF=$(CF) -DMOD=$(MOD) -o temp_prog.o
	ld -r temp_prog.o ckpt.o -o prog.o