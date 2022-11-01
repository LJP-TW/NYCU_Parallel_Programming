CXX := g++
CXXFLAGS := -I./common -O3 -std=c++17 -Wall

ifeq (/usr/bin/g++-10,$(wildcard /usr/bin/g++-10*))
    CXX=g++-10
endif

all: myexp

logger.o: logger.cpp logger.h PPintrin.h PPintrin.cpp def.h
	$(CXX) $(CXXFLAGS) -c logger.cpp

PPintrin.o: PPintrin.cpp PPintrin.h logger.cpp logger.h def.h
	$(CXX) $(CXXFLAGS) -c PPintrin.cpp

myexp: PPintrin.o logger.o main.cpp serialOP.cpp vectorOP.cpp 
	g++  -I./common logger.o PPintrin.o main.cpp serialOP.cpp vectorOP.cpp -o myexp

clean:
	rm -f *.o *.s myexp *~
