# Host-side tests for the platform-agnostic geometry. No console toolchain needed.
CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra

all: out geometry_test paint_test layout_test
	./geometry_test
	./paint_test
	./layout_test

# Also runs the real container and window classes (needs FreeType headers; see below)
elements: stubs elements_test
	./elements_test

out:
	mkdir -p out

geometry_test: test_geometry.cpp Raster.h ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiGeometry.h
	$(CXX) $(CXXFLAGS) -o $@ test_geometry.cpp ../source/libgui/GuiGeometry.cpp

paint_test: test_paint.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiPaint.h ../source/libgui/GuiTheme.h
	$(CXX) $(CXXFLAGS) -o $@ test_paint.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGeometry.cpp

layout_test: test_layout.cpp ../source/libgui/GuiLayoutSolver.cpp ../source/libgui/GuiLayoutSolver.h
	$(CXX) $(CXXFLAGS) -o $@ test_layout.cpp ../source/libgui/GuiLayoutSolver.cpp ../source/libgui/GuiGeometry.cpp

# Real GuiRow/GuiColumn/GuiWindow on the host. Needs FreeType headers (Gui.h includes the text
# renderer) and stand-ins for the asset headers the build normally generates from data/.
FT_CFLAGS ?= $(shell pkg-config --cflags freetype2 2>/dev/null || echo -I/usr/include/freetype2)
ELEMENTS_SRC = ../source/libgui/GuiLayout.cpp ../source/libgui/GuiWindow.cpp ../source/libgui/GuiElement.cpp \
	../source/libgui/GuiLayoutSolver.cpp ../source/libgui/GuiGeometry.cpp

stubs: ../source/filelist.h
	mkdir -p stubs
	for h in $$(sed -n 's/^\#include "\(.*\)\.h"/\1/p' ../source/filelist.h); do \
		printf '#pragma once\nextern const unsigned char %s[];\nextern const unsigned int %s_size;\n' $$h $$h > stubs/$$h.h; done

elements_test: test_layout_elements.cpp $(ELEMENTS_SRC)
	$(CXX) $(CXXFLAGS) -I../source $(FT_CFLAGS) -Istubs -o $@ test_layout_elements.cpp $(ELEMENTS_SRC)

clean:
	rm -rf geometry_test paint_test layout_test elements_test stubs out

.PHONY: all elements stubs clean
