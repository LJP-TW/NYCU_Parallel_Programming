TARGET := mpi_hello pi_block_linear pi_block_tree pi_nonblock_linear pi_gather pi_reduce

MPI_HELLO_C_FILES = hello.cc
PI_BLOCK_LINEAR_SRC_FILES = pi_block_linear.cc
PI_BLOCK_TREE_SRC_FILES = pi_block_tree.cc
PI_NONBLOCK_LINEAR_SRC_FILES = pi_nonblock_linear.cc
PI_GATHER_SRC_FILES = pi_gather.cc
PI_REDUCE_SRC_FILES = pi_reduce.cc

all: $(TARGET)
#	Copy to all hosts
	parallel-scp -A -r -h ../setting/hosts.txt ~/HW4 ~

mpi_hello: $(MPI_HELLO_C_FILES)
# 	Compile
	mpicxx $< -o $@

pi_block_linear: $(PI_BLOCK_LINEAR_SRC_FILES)
	mpicxx $< -o $@

pi_block_tree: $(PI_BLOCK_TREE_SRC_FILES)
	mpicxx $< -o $@

pi_nonblock_linear: $(PI_NONBLOCK_LINEAR_SRC_FILES)
	mpicxx $< -o $@

pi_gather: $(PI_GATHER_SRC_FILES)
	mpicxx $< -o $@

pi_reduce: $(PI_REDUCE_SRC_FILES)
	mpicxx $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)

.PHONY: report
report: $(TARGET)
	python3 ./test.py
