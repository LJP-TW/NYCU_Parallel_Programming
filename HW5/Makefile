NVCC = nvcc
CXX=g++
CXXFLAGS=-I./common -Iobjs/ -O3 -std=c++17 -Wall -g -fPIC -lm

APP_NAME=mandelbrot
OBJDIR=objs
COMMONDIR=./common

CUDA_LINK_FLAGS =  -rdc=true -gencode=arch=compute_61,code=sm_61 -Xcompiler '-fPIC' 
CUDA_COMPILE_FLAGS = --device-c -gencode=arch=compute_61,code=sm_61 -Xcompiler '-fPIC' -g -O3

PPM_CXX=$(COMMONDIR)/ppm.cpp
PPM_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(PPM_CXX:.cpp=.o)))


default: $(APP_NAME)

.PHONY: dirs clean

dirs:
		/bin/mkdir -p $(OBJDIR)/

clean:
		/bin/rm -rf $(OBJDIR) *.ppm *~ $(APP_NAME)

OBJS=$(OBJDIR)/main.o $(OBJDIR)/kernel1.o $(OBJDIR)/kernel2.o $(OBJDIR)/kernel3.o $(OBJDIR)/kernel4.o \
     $(OBJDIR)/mandelbrotSerial.o $(OBJDIR)/mandelbrotThread.o $(PPM_OBJ)

$(APP_NAME): dirs $(OBJS)
		$(NVCC) ${CUDA_LINK_FLAGS} -o $@ $(OBJS) mandelbrotThreadRef.a

$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/main.o: $(COMMONDIR)/CycleTimer.h kernel.h

$(OBJDIR)/kernel1.o : kernel1.cu kernel.h
	${NVCC} ${CUDA_COMPILE_FLAGS} -c $< -o $@

$(OBJDIR)/kernel2.o : kernel2.cu kernel.h
	${NVCC} ${CUDA_COMPILE_FLAGS} -c $< -o $@

$(OBJDIR)/kernel3.o : kernel3.cu kernel.h
	${NVCC} ${CUDA_COMPILE_FLAGS} -c $< -o $@

$(OBJDIR)/kernel4.o : kernel4.cu kernel.h
	${NVCC} ${CUDA_COMPILE_FLAGS} -c $< -o $@
