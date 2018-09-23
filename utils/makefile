all: createdb

createdb: createdb.cpp
	g++ -std=c++17 -Wall -ocreatedb  createdb.cpp -lstdc++fs

clean:
	rm -f *o createdb
