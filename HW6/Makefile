default: conv

CC = gcc-10
FLAGS = -O3 -lOpenCL -m64 -ffloat-store -w -g

OBJS = main.o bmpfuncs.o hostFE.o serialConv.o helper.o

conv: $(OBJS)
	$(CC) -o $@ $(OBJS) $(FLAGS)

%.o: %.c
	$(CC) -c $(FLAGS) $< -o $@

clean:
	rm -f conv *.o output.bmp ref.bmp