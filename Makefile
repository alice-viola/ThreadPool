CC=g++ -std=c++11 -Wall
OPT=-O3
OPTL= -pthread
DEPS=threadpool.hpp test.hpp
OBJ=main.o 
INCLUDE=-I/usr/local/include/
LIBS_PATH=-L/usr/local/lib/
LIBS= -lcppunit-1.14.0
CPP_UNIT=`pkg-config --libs cppunit`

%.o: %.cpp $(DEPS)
	$(CC) $(OPT)  -c -o $@ $< $(CFLAGS) $(INCLUDE)

ThreadPool: $(OBJ)
	$(CC) $(OPT)  -o $@ $^ $(CFLAGS) $(INCLUDE) $(LIBS_PATH) $(LIBS)

ThreadPoolTest: $(OBJ)
	$(CC) $(OPT) $(OPTL) -o $@ $^ $(CFLAGS) $(INCLUDE) $(CPP_UNIT)

.PHONY: clean

clean:
	rm -f *.o 
