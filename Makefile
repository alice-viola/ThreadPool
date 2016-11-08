CC=g++ -std=c++14
CXXFLAGS=-I.
OPT=-O3
DEPS=threadpool.hpp
OBJ=main.o 

%.o: %.cpp $(DEPS)
	$(CC) $(OPT)  -c -o $@ $< $(CFLAGS)

ThreadPool: $(OBJ)
	$(CC) $(OPT)  -o $@ $^ $(CFLAGS) $(OPENCV)

.PHONY: clean

clean:
	rm -f *.o 
