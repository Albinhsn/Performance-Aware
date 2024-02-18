LD_FLAGS := -lm


r:
	gcc -O2 main.c ./haversine.c ./lib/json.c ./lib/files.c $(LD_FLAGS)  -o main && ./main

gen:
	gcc -O2 generate.c ./haversine.c ./lib/json.c ./lib/files.c $(LD_FLAGS)  -o generate && ./generate cluster 140242410 10000000
