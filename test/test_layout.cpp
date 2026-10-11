// Host-side tests for the layout solver and selection navigation. Run with `make -C test`.
#include "../source/libgui/GuiLayoutSolver.h"
#include <cstdio>
#include <cmath>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)
#define NEAR(a, b) (fabsf((a) - (b)) < 0.01f)
#define RECT_IS(r, X, Y, W, H) (NEAR((r).x, X) && NEAR((r).y, Y) && NEAR((r).w, W) && NEAR((r).h, H))
#define SHOW(r) (r).x, (r).y, (r).w, (r).h

static GuiStackItem item(float w, float h, float grow = 0) { GuiStackItem i; i.w = w; i.h = h; i.grow = grow; return i; }
static GuiNavItem nav(float x, float y, float w, float h, bool sel = true) { GuiNavItem n; n.r = GuiRect{ x, y, w, h }; n.selectable = sel; return n; }

int main()
{
	// ---- row, fit to content
	{
		GuiStackSpec s; s.spacing = 8;
		std::vector<GuiStackItem> v = { item(50, 20), item(30, 40), item(10, 10) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.width, 50 + 30 + 10 + 16) && NEAR(r.height, 40), "row fits: %.1f x %.1f", r.width, r.height);
		CHECK(RECT_IS(r.rects[0], 0, 0, 50, 20), "first at origin: %.1f,%.1f %.1fx%.1f", SHOW(r.rects[0]));
		CHECK(RECT_IS(r.rects[1], 58, 0, 30, 40), "second after spacing");
		CHECK(RECT_IS(r.rects[2], 96, 0, 10, 10), "third");
	}
	// ---- padding
	{
		GuiStackSpec s; s.padding = GuiInsets(10, 5);
		std::vector<GuiStackItem> v = { item(40, 20) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.width, 60) && NEAR(r.height, 30), "padding grows the box: %.1f x %.1f", r.width, r.height);
		CHECK(RECT_IS(r.rects[0], 10, 5, 40, 20), "child inset by the padding");
		GuiStackSpec s2; s2.padding = GuiInsets(1, 2, 3, 4);
		GuiLayoutResult r2 = GuiSolveStack(s2, v);
		CHECK(NEAR(r2.width, 44) && NEAR(r2.height, 26) && RECT_IS(r2.rects[0], 1, 2, 40, 20), "per-side padding");
	}
	// ---- column mirrors the row
	{
		GuiStackSpec s; s.vertical = true; s.spacing = 4;
		std::vector<GuiStackItem> v = { item(50, 20), item(30, 40) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.width, 50) && NEAR(r.height, 64), "column fits: %.1f x %.1f", r.width, r.height);
		CHECK(RECT_IS(r.rects[1], 0, 24, 30, 40), "second below the first");
	}
	// ---- cross-axis alignment
	{
		GuiStackSpec s; s.alignItems = LAYOUT_ALIGN::CENTRE;
		std::vector<GuiStackItem> v = { item(10, 20), item(10, 40) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].y, 10) && NEAR(r.rects[1].y, 0), "centered across: y=%.1f / %.1f", r.rects[0].y, r.rects[1].y);
		s.alignItems = LAYOUT_ALIGN::END;
		r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].y, 20), "aligned to the end");
		s.alignItems = LAYOUT_ALIGN::STRETCH;
		r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].h, 40) && NEAR(r.rects[1].h, 40), "stretched to the tallest");
		v[0].useContainerAlign = false; v[0].align = LAYOUT_ALIGN::START;
		r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].h, 20) && NEAR(r.rects[1].h, 40), "per-item alignment overrides the container's");
	}
	// ---- fixed size and justify
	{
		GuiStackSpec s; s.fixedW = 200; s.fixedH = 50;
		std::vector<GuiStackItem> v = { item(40, 10), item(40, 10) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.width, 200) && NEAR(r.height, 50) && NEAR(r.rects[0].x, 0), "fixed size, START");
		s.justify = LAYOUT_JUSTIFY::CENTRE; r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].x, 60) && NEAR(r.rects[1].x, 100), "CENTRE: %.1f %.1f", r.rects[0].x, r.rects[1].x);
		s.justify = LAYOUT_JUSTIFY::END; r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[1].x, 160), "END");
		s.justify = LAYOUT_JUSTIFY::SPACE_BETWEEN; r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].x, 0) && NEAR(r.rects[1].x, 160), "SPACE_BETWEEN: %.1f %.1f", r.rects[0].x, r.rects[1].x);
		std::vector<GuiStackItem> one = { item(40, 10) };
		r = GuiSolveStack(s, one);
		CHECK(NEAR(r.rects[0].x, 0), "SPACE_BETWEEN with one item stays at the start");
	}
	// ---- grow shares the leftover by weight
	{
		GuiStackSpec s; s.fixedW = 300; s.spacing = 10;
		std::vector<GuiStackItem> v = { item(40, 10), item(20, 10, 1), item(20, 10, 2) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		// leftover = 300 - (40+20+20) - 20 = 200 -> 66.67 and 133.33
		CHECK(NEAR(r.rects[0].w, 40), "fixed item keeps its size");
		CHECK(NEAR(r.rects[1].w, 20 + 200.0f / 3.0f) && NEAR(r.rects[2].w, 20 + 400.0f / 3.0f), "weighted growth: %.2f %.2f", r.rects[1].w, r.rects[2].w);
		CHECK(NEAR(r.rects[2].x + r.rects[2].w, 300), "growing items fill the container exactly: ends at %.2f", r.rects[2].x + r.rects[2].w);
		s.justify = LAYOUT_JUSTIFY::END; // justify is moot once something grows
		GuiLayoutResult r2 = GuiSolveStack(s, v);
		CHECK(NEAR(r2.rects[0].x, 0), "growth wins over justify");
	}
	// ---- the title-bar pattern: label, flexible space, label
	{
		GuiStackSpec s; s.fixedW = 600;
		GuiStackItem space; space.grow = 1;
		std::vector<GuiStackItem> v = { item(100, 24), space, item(150, 24) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].x, 0) && NEAR(r.rects[2].x, 450), "label left, label right: %.1f / %.1f", r.rects[0].x, r.rects[2].x);
	}
	// ---- overflow is not squashed, and nothing divides by zero
	{
		GuiStackSpec s; s.fixedW = 50;
		std::vector<GuiStackItem> v = { item(40, 10, 1), item(40, 10, 1) };
		GuiLayoutResult r = GuiSolveStack(s, v);
		CHECK(NEAR(r.rects[0].w, 40) && NEAR(r.rects[1].x, 40), "content larger than the box overflows");
		GuiLayoutResult e = GuiSolveStack(GuiStackSpec(), std::vector<GuiStackItem>());
		CHECK(NEAR(e.width, 0) && NEAR(e.height, 0) && e.rects.empty(), "empty stack");
		GuiStackSpec z; z.fixedW = 100;
		std::vector<GuiStackItem> zero = { item(10, 10, 0), item(10, 10, -5) };
		GuiLayoutResult rz = GuiSolveStack(z, zero);
		CHECK(std::isfinite(rz.rects[1].x), "negative grow is ignored");
	}

	// ---- grid
	{
		GuiGridSpec g; g.columns = 2; g.spacingX = 10; g.spacingY = 6;
		std::vector<GuiGridItem> v(4);
		v[0].w = 100; v[0].h = 40; v[1].w = 60; v[1].h = 20; v[2].w = 80; v[2].h = 30; v[3].w = 90; v[3].h = 50;
		GuiLayoutResult r = GuiSolveGrid(g, v);
		// column widths 100 and 90, row heights 40 and 50
		CHECK(NEAR(r.width, 100 + 10 + 90) && NEAR(r.height, 40 + 6 + 50), "grid fits: %.1f x %.1f", r.width, r.height);
		CHECK(NEAR(r.rects[1].x, 110) && NEAR(r.rects[2].y, 46) && NEAR(r.rects[3].x, 110), "cells line up in columns and rows");
		v[1].alignH = LAYOUT_ALIGN::CENTRE; v[1].alignV = LAYOUT_ALIGN::END;
		r = GuiSolveGrid(g, v);
		CHECK(NEAR(r.rects[1].x, 110 + 15) && NEAR(r.rects[1].y, 20), "item aligned inside its cell: %.1f,%.1f", r.rects[1].x, r.rects[1].y);
		v[0].alignH = LAYOUT_ALIGN::STRETCH; v[0].alignV = LAYOUT_ALIGN::STRETCH;
		r = GuiSolveGrid(g, v);
		CHECK(NEAR(r.rects[0].w, 100) && NEAR(r.rects[0].h, 40), "stretch fills the cell");
		GuiGridSpec fixed = g; fixed.fixedW = 310;
		r = GuiSolveGrid(fixed, v);
		// columns of (310 - 10) / 2 = 150: the second starts at 160, and the 60-wide centred item sits at 160 + 45
		CHECK(NEAR(r.width, 310) && NEAR(r.rects[1].x, 205), "a fixed width makes equal columns: item at %.1f", r.rects[1].x);
		GuiGridSpec three; three.columns = 3;
		std::vector<GuiGridItem> five(5);
		for(int i = 0; i < 5; i++) { five[i].w = 10; five[i].h = 10; }
		GuiLayoutResult r5 = GuiSolveGrid(three, five);
		CHECK(NEAR(r5.rects[3].x, 0) && NEAR(r5.rects[3].y, 10) && NEAR(r5.height, 20), "5 items in 3 columns make 2 rows");
		CHECK(GuiSolveGrid(three, std::vector<GuiGridItem>()).rects.empty(), "empty grid");
	}

	// ---- overlay
	{
		GuiOverlaySpec o; o.padding = GuiInsets(12);
		std::vector<GuiOverlayItem> v(3);
		v[0].background = true;
		v[1].w = 100; v[1].h = 40;
		v[2].w = 30; v[2].h = 30; v[2].alignH = LAYOUT_ALIGN::END; v[2].alignV = LAYOUT_ALIGN::START;
		GuiLayoutResult r = GuiSolveOverlay(o, v);
		CHECK(NEAR(r.width, 124) && NEAR(r.height, 64), "box fits the largest child plus padding: %.1f x %.1f", r.width, r.height);
		CHECK(RECT_IS(r.rects[0], 0, 0, 124, 64), "a background fills the whole box, ignoring padding");
		CHECK(NEAR(r.rects[1].x, 12) && NEAR(r.rects[1].y, 12), "the widest child sits inside the padding: %.1f,%.1f", r.rects[1].x, r.rects[1].y);
		CHECK(NEAR(r.rects[2].x, 124 - 12 - 30) && NEAR(r.rects[2].y, 12), "a corner badge: %.1f,%.1f", r.rects[2].x, r.rects[2].y);
		v[1].alignH = LAYOUT_ALIGN::STRETCH;
		r = GuiSolveOverlay(o, v);
		CHECK(NEAR(r.rects[1].w, 100), "stretch fills the inner box");
		GuiOverlaySpec fx; fx.fixedW = 200; fx.fixedH = 100;
		std::vector<GuiOverlayItem> one(1); one[0].w = 40; one[0].h = 20;
		r = GuiSolveOverlay(fx, one);
		CHECK(NEAR(r.rects[0].x, 80) && NEAR(r.rects[0].y, 40), "centered by default in a fixed box");
	}

	// ---- navigation: a column
	{
		std::vector<GuiNavItem> v = { nav(0, 0, 100, 30), nav(0, 40, 100, 30), nav(0, 80, 100, 30) };
		CHECK(GuiNavigate(v, 0, NAV_DIR::DOWN) == 1 && GuiNavigate(v, 1, NAV_DIR::DOWN) == 2, "down the column");
		CHECK(GuiNavigate(v, 2, NAV_DIR::UP) == 1 && GuiNavigate(v, 0, NAV_DIR::UP) == -1, "up stops at the top");
		CHECK(GuiNavigate(v, 2, NAV_DIR::DOWN) == -1, "down stops at the bottom");
		CHECK(GuiNavigate(v, -1, NAV_DIR::DOWN) == 0, "nothing selected: first item");
	}
	// ---- navigation: a row whose items have different heights (centred), which exact-row matching cannot handle
	{
		std::vector<GuiNavItem> v = { nav(0, 10, 80, 20), nav(90, 0, 80, 40), nav(180, 15, 80, 10) };
		CHECK(GuiNavigate(v, 0, NAV_DIR::RIGHT) == 1 && GuiNavigate(v, 1, NAV_DIR::RIGHT) == 2, "right along a row with mixed heights");
		CHECK(GuiNavigate(v, 2, NAV_DIR::LEFT) == 1 && GuiNavigate(v, 1, NAV_DIR::LEFT) == 0, "left along the same row");
	}
	// ---- navigation: a grid, including wrap to the next row
	{
		std::vector<GuiNavItem> v = { nav(0, 0, 90, 30), nav(100, 0, 90, 30), nav(0, 40, 90, 30), nav(100, 40, 90, 30) };
		CHECK(GuiNavigate(v, 0, NAV_DIR::DOWN) == 2 && GuiNavigate(v, 1, NAV_DIR::DOWN) == 3, "down keeps the column");
		CHECK(GuiNavigate(v, 3, NAV_DIR::UP) == 1, "up keeps the column");
		CHECK(GuiNavigate(v, 1, NAV_DIR::RIGHT) == 2, "right off the end of a row wraps to the next row's first item");
		CHECK(GuiNavigate(v, 2, NAV_DIR::LEFT) == 1, "left off the start of a row wraps to the previous row's last item");
		CHECK(GuiNavigate(v, 3, NAV_DIR::RIGHT) == -1 && GuiNavigate(v, 0, NAV_DIR::LEFT) == -1, "no wrap past the first or last item");
	}
	// ---- navigation: unselectable items and uneven layouts
	{
		std::vector<GuiNavItem> v = { nav(0, 0, 100, 30), nav(0, 40, 100, 30, false), nav(0, 80, 100, 30) };
		CHECK(GuiNavigate(v, 0, NAV_DIR::DOWN) == 2, "skips unselectable items");
		// a wide button under two narrow ones: going down from either reaches it; going up picks the nearer
		std::vector<GuiNavItem> u = { nav(0, 0, 50, 30), nav(60, 0, 50, 30), nav(0, 40, 110, 30) };
		CHECK(GuiNavigate(u, 0, NAV_DIR::DOWN) == 2 && GuiNavigate(u, 1, NAV_DIR::DOWN) == 2, "a wide item below two narrow ones");
		CHECK(GuiNavigate(u, 2, NAV_DIR::UP) == 0 || GuiNavigate(u, 2, NAV_DIR::UP) == 1, "up from the wide item reaches the row above");
		CHECK(GuiNavigate(std::vector<GuiNavItem>(), 0, NAV_DIR::DOWN) == -1, "no items");
		std::vector<GuiNavItem> none = { nav(0, 0, 10, 10, false) };
		CHECK(GuiNavigate(none, -1, NAV_DIR::DOWN) == -1, "nothing selectable");
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
