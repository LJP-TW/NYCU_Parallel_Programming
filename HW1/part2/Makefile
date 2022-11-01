TARGET := test_auto_vectorize

OBJS := main.o test1.o test2.o test3.o

CXX := clang++

ifeq (/usr/bin/clang++-11,$(wildcard /usr/bin/clang++-11*))
    CXX=clang++-11
endif

CXXFLAGS := -I./common -O3 -std=c++17 -Wall

ifeq ($(ASSEMBLE),1)
	CXXFLAGS += -S
endif
ifeq ($(VECTORIZE),1)
	CXXFLAGS += -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize
	SUFFIX := .vec
else
	CXXFLAGS += -fno-vectorize
	SUFFIX := .novec
endif
ifeq ($(RESTRICT),1)
	SUFFIX := $(SUFFIX).restr
endif
ifeq ($(ALIGN),1)
	SUFFIX := $(SUFFIX).align
endif
ifeq ($(AVX2),1)
	CXXFLAGS += -mavx2
	SUFFIX := $(SUFFIX).avx2
endif
ifeq ($(FASTMATH),1)
  	CXXFLAGS += -ffast-math
  	SUFFIX := $(SUFFIX).fmath
endif

all: $(TARGET)

%.o: %.cpp test.h
ifeq ($(ASSEMBLE),1)
	if [ ! -d "./assembly" ]; then mkdir "./assembly"; fi
	$(CXX) $(CXXFLAGS) -c $< -o assembly/$(basename $<)$(SUFFIX).s
else
	$(CXX) $(CXXFLAGS) -c $< 
endif

$(TARGET): $(OBJS)
ifneq ($(ASSEMBLE),1)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@
endif

clean:
	rm -f *.o *.s $(TARGET) *~

cleanall:
	rm -rf *.o *.s $(TARGET) *~ assembly

