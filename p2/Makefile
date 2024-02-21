LD_FLAGS := -lm


r:
	g++ -O2 main.cpp $(LD_FLAGS)  -o main && ./main

gen:
	g++ -O2 generate.cpp ./haversine.cpp ./lib/json.cpp ./lib/files.cpp $(LD_FLAGS)  -o generate && ./generate cluster 140242410 10000000

rep:
	g++ -O2 ./lib/string.cpp ./lib/common.cpp rep.cpp -o repitition $(LD_FLAGS) && ./repitition
