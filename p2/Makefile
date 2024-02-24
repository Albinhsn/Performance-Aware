LD_FLAGS := -lm


r:
	nasm -f elf64 parseString.asm -o parseString && g++ -pthread -O2 main.cpp parseString.o $(LD_FLAGS)  -o main && ./main

gen:
	g++ -O2 generate.cpp ./lib/common.cpp ./lib/string.cpp ./haversine.cpp ./lib/json.cpp ./lib/files.cpp $(LD_FLAGS)  -o generate && ./generate cluster 140242410 10000000
