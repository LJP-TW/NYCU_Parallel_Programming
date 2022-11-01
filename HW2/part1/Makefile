TARGET := pi.out

C_FILES = $(wildcard *.c)
O_FILES = $(C_FILES:%.c=%.o)

all: $(TARGET)

$(TARGET): $(O_FILES)
	gcc $^ -o $@

%.o: %.c
	gcc -c $< -O3 -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
	rm -f cpu.txt result.txt time.txt

.PHONY: report
report: $(TARGET)
	lscpu | grep -E "name|Flags" > cpu.txt
	cat /proc/cpuinfo | grep MHz >> cpu.txt
	bash -c "{ time (./pi.out 3 100000000; ./pi.out 4 100000000) >result.txt ; } 2>time.txt"