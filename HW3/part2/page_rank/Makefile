all: default grade

default: page_rank.cpp main.cpp
	g++ -I../ -std=c++17 -fopenmp -O3 -o pr main.cpp page_rank.cpp ../common/graph.cpp ref_pr.a
grade: page_rank.cpp grade.cpp
	g++ -I../ -std=c++17 -fopenmp -O3 -o pr_grader grade.cpp page_rank.cpp ../common/graph.cpp ref_pr.a
clean:
	rm -rf pr pr_grader *~ *.*~
report: clean all
	./pr_grader ../graphs > report.txt