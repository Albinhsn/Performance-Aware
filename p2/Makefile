LD_FLAGS := -lm


r:
	g++ -O2 arena.cpp main.cpp $(LD_FLAGS)  -o main && ./main

gen:
	g++ -O2 generate.cpp ./lib/common.cpp ./lib/string.cpp ./haversine.cpp ./lib/json.cpp ./lib/files.cpp $(LD_FLAGS)  -o generate && ./generate cluster 140242410 10000000
