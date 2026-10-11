# Host-side tests for the platform-agnostic geometry. No console toolchain needed.
CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra

all: out geometry_test paint_test
	./geometry_test
	./paint_test

out:
	mkdir -p out

geometry_test: test_geometry.cpp Raster.h ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiGeometry.h
	$(CXX) $(CXXFLAGS) -o $@ test_geometry.cpp ../source/libgui/GuiGeometry.cpp

paint_test: test_paint.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiPaint.h ../source/libgui/GuiTheme.h
	$(CXX) $(CXXFLAGS) -o $@ test_paint.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGeometry.cpp

clean:
	rm -rf geometry_test paint_test out

.PHONY: all clean
