LD_FLAGS := -lm

asm2:
	nasm -f elf64 nop_loop.asm -o nop_loop.o && g++ -O0 ../p2/lib/string.cpp ../p2/lib/common.cpp rep_asm2.cpp nop_loop.o -o rep_assembly2 $(LD_FLAGS) && ./rep_assembly2

asm:
	nasm -f elf64 nop_loop.asm -o nop_loop.o && g++ -O0 ../p2/lib/string.cpp ../p2/lib/common.cpp rep_asm.cpp nop_loop.o -o rep_assembly $(LD_FLAGS) && ./rep_assembly
rep:
	g++ -O2 ../p2/lib/string.cpp ../p2/lib/common.cpp rep.cpp -o repitition $(LD_FLAGS) && ./repitition
