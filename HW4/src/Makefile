TARGET := mpi_hello pi_block_linear pi_block_tree

MPI_HELLO_C_FILES = hello.cc
PI_BLOCK_LINEAR_SRC_FILES = pi_block_linear.cc
PI_BLOCK_TREE_SRC_FILES = pi_block_tree.cc

all: $(TARGET)
#	Copy to all hosts
	parallel-scp -A -r -h hosts.txt ~/HW4 ~

mpi_hello: $(MPI_HELLO_C_FILES)
# 	Compile
	mpicxx $< -o $@

pi_block_linear: $(PI_BLOCK_LINEAR_SRC_FILES)
	mpicxx $< -o $@

pi_block_tree: $(PI_BLOCK_TREE_SRC_FILES)
	mpicxx $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)

.PHONY: report
report: $(TARGET)
	python3 ./test.py
