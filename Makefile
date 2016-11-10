CC=g++ -std=c++14
OPT=-O3
DEPS=threadpool.hpp test.hpp example.hpp
OBJ=main.o 

%.o: %.cpp $(DEPS)
	$(CC) $(OPT)  -c -o $@ $< $(CFLAGS)

ThreadPool: $(OBJ)
	$(CC) $(OPT)  -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o 
