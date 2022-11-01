TARGET := pi.out

CC := clang

C_FILES = $(wildcard *.c)
O_FILES = $(C_FILES:%.c=%.o)

CXXFLAGS := -O3 -mavx2 -ffast-math -march=native -Wall
LDFLAGS  := -lpthread

all: $(TARGET)

$(TARGET): $(O_FILES)
	$(CC) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CXXFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
	rm -f report.txt

.PHONY: report
report: clean $(TARGET)
	lscpu | grep -E "name|Flags" > report.txt
	cat /proc/cpuinfo | grep MHz >> report.txt
	bash -c "{ time (./pi.out 3 100000000; ./pi.out 4 100000000) >>report.txt ; } 2>>report.txt"