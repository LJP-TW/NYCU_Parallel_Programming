TARGET := matmul

CC_FILES = $(wildcard *.cc)
O_FILES = $(CC_FILES:%.cc=%.o)

all: $(TARGET)

$(TARGET): $(O_FILES)
	mpicxx $^ -o $@

%.o: %.cc
	mpicxx -O3 -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)

.PHONY: sync
sync: all
#	Copy to all hosts
	parallel-scp -A -r -h ../setting/hosts.txt ~/HW4 ~

.PHONY: report
report: sync
	python3 ./test.py