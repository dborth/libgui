# Host-side tests for the platform-agnostic geometry. No console toolchain needed.
CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra

all: out geometry_test
	./geometry_test

out:
	mkdir -p out

geometry_test: test_geometry.cpp Raster.h ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiGeometry.h
	$(CXX) $(CXXFLAGS) -o $@ test_geometry.cpp ../source/libgui/GuiGeometry.cpp

clean:
	rm -rf geometry_test out

.PHONY: all clean
