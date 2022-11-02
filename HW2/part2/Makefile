
CXX=g++ -m64
CXXFLAGS=-I./common -Iobjs/ -O3 -std=c++17 -Wall

APP_NAME=mandelbrot
OBJDIR=objs
COMMONDIR=./common

PPM_CXX=$(COMMONDIR)/ppm.cpp
PPM_OBJ=$(addprefix $(OBJDIR)/, $(subst $(COMMONDIR)/,, $(PPM_CXX:.cpp=.o)))


default: $(APP_NAME)

.PHONY: dirs clean

dirs:
		/bin/mkdir -p $(OBJDIR)/

clean:
		/bin/rm -rf $(OBJDIR) *.ppm *~ $(APP_NAME)

OBJS=$(OBJDIR)/main.o $(OBJDIR)/mandelbrotSerial.o $(OBJDIR)/mandelbrotThread.o $(PPM_OBJ)

$(APP_NAME): dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm -lpthread

$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/main.o: $(COMMONDIR)/CycleTimer.h

.PHONY: report
report: clean $(APP_NAME)
	lscpu | grep -E "name|Flags" > report.txt
	cat /proc/cpuinfo | grep MHz >> report.txt
	bash -c "./mandelbrot -t 3 >> report.txt ; ./mandelbrot -t 4 >> report.txt"