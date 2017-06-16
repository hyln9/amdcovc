###
# Makefile
# Mateusz Szpakowski
# Virgil Hou
###

ARCH := $(shell getconf LONG_BIT)
ADLSDKDIR := $(HOME)/ADL_SDK9

ifeq ($(ARCH),64)
	APPSDKLIB := $(AMDAPPSDKROOT)/lib/x86_64
else
	APPSDKLIB := $(AMDAPPSDKROOT)/lib/x86
endif

CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11
LDFLAGS = -Wall -O2 -std=c++11
INCDIRS = -I$(ADLSDKDIR)/include
LIBDIRS = -L$(APPSDKLIB)
LIBS = -ldl -lpci -lm -lOpenCL -pthread

.PHONY: all clean

all: build/amdcovc

build/amdcovc: build/amdcovc.o
	$(CXX) $(LDFLAGS) $(LIBDIRS) -o $@ $^ $(LIBS)

build/%.o: %.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c -o $@ $<

clean:
	rm -rf build

