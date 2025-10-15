WRITES=950
READS=50
CF=0

all:
	gcc -c prog.c -funroll-loops -pie -fPIE  -I ../include -DWRITES=$(WRITES) -DREADS=$(READS) -DCF=$(CF)
