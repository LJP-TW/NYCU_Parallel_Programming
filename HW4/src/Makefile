TARGET := mpi_hello

MPI_HELLO_C_FILES = hello.cc

all: $(TARGET)
#	Copy to all hosts
	parallel-scp -A -r -h hosts.txt ~/HW4 ~

mpi_hello: $(MPI_HELLO_C_FILES)
# 	Compile
	mpicxx $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)

.PHONY: report
report: $(TARGET)
	python3 ./test.py
