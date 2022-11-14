SHELL=/bin/sh
BENCHMARK=cg
BENCHMARKU=CG
PROGRAMNAME=cg
DATASIZE=MEDIUMN

default: ${PROGRAMNAME} grade

include make.common

OBJS = cg_impl.o \
       ${COMMON}/${RAND}.o \
       ${COMMON}/c_timers.o \
       ${COMMON}/wtime.o

${PROGRAMNAME}: config ${PROGRAMNAME}.o ${OBJS}
	${CLINK} ${CLINKFLAGS} -Wl,--allow-multiple-definition -o ${PROGRAMNAME} ${PROGRAMNAME}.o ${OBJS} ${C_LIB}

grade: config grade.o ${OBJS}
	${CLINK} ${CLINKFLAGS} -Wl,--allow-multiple-definition -o cg_grader grade.o ${OBJS} ref_cg.a def_cg.a ${C_LIB}

.c.o:
	${CCOMPILE} $< -D${DATASIZE}

cg.o:	cg.c  globals.h
cg_impl.o:	cg_impl.c  globals.h

clean:
	- rm -f *.o *~
	rm -f ${COMMON}/*.o
	rm -f ${PROGRAMNAME} cg_grader
	rm -f gmon.out

profiling: CFLAGS += -pg -Wall
profiling: CLINKFLAGS += -pg -Wall
profiling: clean ${PROGRAMNAME}
	./${PROGRAMNAME}
	gprof ${PROGRAMNAME} gmon.out -b > profiling_result
	sudo perf record -e cpu-cycles ./${PROGRAMNAME}
# 	sudo perf report -F+period,srcline

report: clean ${PROGRAMNAME}
	lscpu | grep -E "name|Flags" > report.txt
	cat /proc/cpuinfo | grep MHz >> report.txt
	./cg_grader >> report.txt