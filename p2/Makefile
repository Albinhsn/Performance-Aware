LD_FLAGS := -lm


r:
	gcc -O2 main.c ./lib/json.c ./lib/files.c $(LD_FLAGS)  -o main && ./main

generate:
	gcc -O2 main.c ./lib/json.c ./lib/files.c $(LD_FLAGS)  -o main && ./main cluster 140 10000000
