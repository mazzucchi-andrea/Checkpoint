MEM_SIZE=0x100000UL
WRITES=950
READS=50
CF=0

all:
	gcc -c prog.c -funroll-loops -pie -fPIE  -I ../include -DMEM_SIZE=$(MEM_SIZE) -DWRITES=$(WRITES) -DREADS=$(READS) -DCF=$(CF)
