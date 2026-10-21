# Host-side tests for the platform-agnostic geometry. No console toolchain needed.
CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra

all: out geometry_test paint_test layout_test controller_test
	./geometry_test
	./paint_test
	./layout_test
	./controller_test

controller_test: test_controller.cpp Raster.h ../source/controllerdraw.cpp ../source/controllerlayout.cpp ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGlyph.h
	$(CXX) $(CXXFLAGS) -o $@ test_controller.cpp ../source/controllerdraw.cpp ../source/controllerlayout.cpp ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp

# Also runs the real container and window classes (needs FreeType headers; see below)
elements: stubs elements_test ui_test popover_test imagedata_test
	./elements_test
	./ui_test
	./popover_test
	./imagedata_test

out:
	mkdir -p out

geometry_test: test_geometry.cpp Raster.h ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiGeometry.h
	$(CXX) $(CXXFLAGS) -o $@ test_geometry.cpp ../source/libgui/GuiGeometry.cpp

paint_test: test_paint.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGeometry.cpp ../source/libgui/GuiPaint.h ../source/libgui/GuiTheme.h
	$(CXX) $(CXXFLAGS) -o $@ test_paint.cpp ../source/libgui/GuiPaint.cpp ../source/libgui/GuiTheme.cpp ../source/libgui/GuiGeometry.cpp

layout_test: test_layout.cpp ../source/libgui/GuiLayoutSolver.cpp ../source/libgui/GuiLayoutSolver.h
	$(CXX) $(CXXFLAGS) -o $@ test_layout.cpp ../source/libgui/GuiLayoutSolver.cpp ../source/libgui/GuiGeometry.cpp

# The real container, builder and window classes on the host. Needs FreeType headers (Gui.h includes
# the text renderer) and stand-ins for the asset headers the build normally generates from data/.
# test_ui is built with AddressSanitizer so teardown-order mistakes fail loudly.
FT_CFLAGS ?= $(shell pkg-config --cflags freetype2 2>/dev/null || echo -I/usr/include/freetype2)
# Mutex.cpp is the real RAII wrapper; with platform == nullptr (host_stubs.cpp) it is a harmless no-op,
# which is all GuiMeshCache's lock needs in these single-threaded tests.
HOST_SRC = host_stubs.cpp ../source/drivers/Mutex.cpp $(addprefix ../source/libgui/, GuiLayout.cpp GuiWindow.cpp GuiElement.cpp GuiLayoutSolver.cpp \
	GuiGeometry.cpp GuiUi.cpp GuiButton.cpp GuiShape.cpp GuiPaint.cpp GuiTheme.cpp GuiText.cpp \
	GuiGlyph.cpp GuiMeshCache.cpp GuiSound.cpp GuiPopover.cpp)

stubs: ../source/filelist.h
	mkdir -p stubs
	for h in $$(sed -n 's/^\#include "\(.*\)\.h"/\1/p' ../source/filelist.h); do \
		printf '#pragma once\nextern const unsigned char %s[];\nextern const unsigned int %s_size;\n' $$h $$h > stubs/$$h.h; done

elements_test: test_layout_elements.cpp $(HOST_SRC)
	$(CXX) $(CXXFLAGS) -I../source $(FT_CFLAGS) -Istubs -o $@ test_layout_elements.cpp $(HOST_SRC)

CONTROLLER_SRC = $(addprefix ../source/, controllerdemo.cpp controllerview.cpp controllerdraw.cpp controllerlayout.cpp)

popover_test: test_popover.cpp $(HOST_SRC)
	$(CXX) $(CXXFLAGS) -I../source $(FT_CFLAGS) -Istubs -o $@ test_popover.cpp $(HOST_SRC)

# GuiImageData against a fake renderer, with PNGs built in memory (needs libpng; it brings its own fake platform, so it
# does not use host_stubs.cpp)
imagedata_test: test_imagedata.cpp ../source/libgui/GuiImageData.cpp ../source/libgui/GuiImageData.h ../source/drivers/Mutex.cpp
	$(CXX) $(CXXFLAGS) -I../source $(FT_CFLAGS) -Istubs -o $@ test_imagedata.cpp ../source/libgui/GuiImageData.cpp ../source/drivers/Mutex.cpp -lpng -lz

ui_test: test_ui.cpp ../source/layoutdemo.cpp ../source/layoutdemo.h $(CONTROLLER_SRC) ../source/controllerdemo.h ../source/controllerview.h $(HOST_SRC)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -I../source $(FT_CFLAGS) -Istubs -o $@ test_ui.cpp ../source/layoutdemo.cpp $(CONTROLLER_SRC) $(HOST_SRC)

clean:
	rm -rf geometry_test paint_test layout_test controller_test popover_test elements_test ui_test imagedata_test stubs out

.PHONY: all elements stubs clean
