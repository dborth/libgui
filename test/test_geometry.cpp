// Host-side tests for GuiGeometry. Build and run with `make -C test`.
#include "../source/libgui/GuiGeometry.h"
#include "Raster.h"
#include <cstdlib>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

static PixelColor rgba(int r, int g, int b, int a = 255) { PixelColor c = { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a }; return c; }
static const float kPi = 3.14159265f;

// Total "ink": integral of alpha over the mesh. Alpha is linear across a triangle,
// so each triangle contributes area * mean vertex alpha.
static double ink(const GuiMesh& m)
{
	const std::vector<MeshVertex>& v = m.vertices();
	double sum = 0;
	for(size_t i = 0; i + 2 < v.size(); i += 3)
	{
		double area = 0.5 * ((v[i+1].x - v[i].x) * (v[i+2].y - v[i].y) - (v[i+2].x - v[i].x) * (v[i+1].y - v[i].y));
		sum += fabs(area) * (v[i].c.a + v[i+1].c.a + v[i+2].c.a) / (3.0 * 255.0);
	}
	return sum;
}

// Structure every mesh must have: finite, whole triangles, one winding.
static void checkSane(const GuiMesh& m, const char* what)
{
	const std::vector<MeshVertex>& v = m.vertices();
	CHECK(v.size() % 3 == 0, "%s: %zu vertices", what, v.size());
	bool finite = true;
	int wrong = 0;
	for(size_t i = 0; i < v.size(); i++)
		if(!std::isfinite(v[i].x) || !std::isfinite(v[i].y)) finite = false;
	for(size_t i = 0; i + 2 < v.size(); i += 3)
	{
		double s = (v[i+1].x - v[i].x) * (v[i+2].y - v[i].y) - (v[i+2].x - v[i].x) * (v[i+1].y - v[i].y);
		if(s < -1e-3) { wrong++; if(wrong == 1) printf("  bad tri %s: (%.3f,%.3f) (%.3f,%.3f) (%.3f,%.3f) s=%g\n", what, v[i].x, v[i].y, v[i+1].x, v[i+1].y, v[i+2].x, v[i+2].y, s); } // positive = clockwise on screen; tiny negatives are float noise on zero-area triangles
	}
	CHECK(finite, "%s: non-finite vertex", what);
	CHECK(wrong == 0, "%s: %d triangles wound the wrong way", what, wrong);
}

static void expectInk(const GuiMesh& m, double expected, double relTol, const char* what)
{
	double got = ink(m);
	CHECK(fabs(got - expected) <= relTol * expected, "%s: ink %.3f expected %.3f (%.3f%%)", what, got, expected, 100.0 * (got - expected) / expected);
}

int main()
{
	const GuiFill white = GuiFill::solid(rgba(255, 255, 255));
	const float f = 1.0f;

	// Fill coverage equals the analytic area (profile is centered on the edge, so the
	// feather adds only pi*(f/2)^2 for round corners, f^2 for sharp ones).
	{
		GuiMesh m(f); GuiRect r = { 10, 10, 100, 60 };
		m.addRect(r, white);
		checkSane(m, "rect");
		expectInk(m, 100.0 * 60.0 + f * f, 0.005, "rect");
	}
	{
		GuiMesh m(f); GuiRect r = { 10, 10, 100, 60 };
		m.addRoundRect(r, GuiCorners(12), white);
		checkSane(m, "roundrect");
		expectInk(m, 100.0 * 60.0 - (4 - kPi) * 144.0, 0.005, "roundrect");
	}
	{
		GuiMesh m(f); GuiRect r = { 10, 10, 80, 40 };
		m.addRoundRect(r, GuiCorners(20), white); // capsule
		checkSane(m, "capsule");
		expectInk(m, 80.0 * 40.0 - (4 - kPi) * 400.0, 0.005, "capsule");
	}
	{
		GuiMesh m(f); GuiRect r = { 0, 0, 50, 50 };
		m.addEllipse(r, white);
		checkSane(m, "circle");
		expectInk(m, kPi * 625.0, 0.005, "circle");
	}
	{
		GuiMesh m(f); GuiRect r = { 0, 0, 80, 30 };
		m.addEllipse(r, white);
		checkSane(m, "ellipse");
		expectInk(m, kPi * 40.0 * 15.0, 0.01, "ellipse");
	}
	{
		GuiMesh m(f); std::vector<GuiPoint> t = { {0, 0}, {60, 0}, {30, 50} }; // given clockwise
		m.addPolygon(t, white);
		checkSane(m, "triangle cw");
		GuiMesh m2(f); std::vector<GuiPoint> t2 = { {30, 50}, {60, 0}, {0, 0} }; // reversed input
		m2.addPolygon(t2, white);
		checkSane(m2, "triangle ccw input");
		expectInk(m, 0.5 * 60 * 50, 0.04, "triangle");
		CHECK(m.size() == m2.size(), "orientation of input must not change the mesh");
	}

	// Strokes: coverage = outer area - inner area for each alignment.
	{
		GuiRect r = { 10, 10, 100, 60 }; float rad = 12, w = 4;
		double A = 100.0 * 60.0 - (4 - kPi) * rad * rad;
		double inner = (100 - 2 * w) * (60 - 2 * w) - (4 - kPi) * (rad - w) * (rad - w);
		GuiMesh m(f); m.strokeRoundRect(r, GuiCorners(rad), w, STROKE::INSIDE, white);
		checkSane(m, "stroke inside");
		expectInk(m, A - inner, 0.01, "stroke inside");
		GuiMesh m2(f); m2.strokeRoundRect(r, GuiCorners(rad), w, STROKE::OUTSIDE, white);
		checkSane(m2, "stroke outside");
		double outer = (100 + 2 * w) * (60 + 2 * w) - (4 - kPi) * (rad + w) * (rad + w) + 0.0;
		// outside offset of a rounded rect with r>0: perimeter*w + pi*w^2 added to A
		double per = 2 * (100 - 2 * rad) + 2 * (60 - 2 * rad) + 2 * kPi * rad;
		outer = A + per * w + kPi * w * w;
		expectInk(m2, outer - A, 0.01, "stroke outside");
		GuiMesh m3(f); m3.strokeRoundRect(r, GuiCorners(rad), w, STROKE::CENTRE, white);
		checkSane(m3, "stroke centre");
		expectInk(m3, per * w, 0.01, "stroke centre"); // centre band: area ~ perimeter * width
	}
	{
		GuiMesh m(f); GuiRect r = { 0, 0, 60, 60 };
		m.strokeEllipse(r, 3, STROKE::INSIDE, white);
		checkSane(m, "ring");
		expectInk(m, kPi * (900.0 - 27.0 * 27.0), 0.01, "ring");
	}
	{
		// A stroke thinner than the feather keeps its coverage (as a fainter, wider line)
		double per = 2 * (100 - 16) + 2 * (40 - 16) + 2 * kPi * 8;
		GuiRect r = { 0, 0, 100, 40 };
		GuiMesh m(f); m.strokeRoundRect(r, GuiCorners(8), 0.5f, STROKE::INSIDE, white);
		checkSane(m, "hairline 0.5");
		expectInk(m, per * 0.5, 0.03, "hairline 0.5");
		GuiMesh q(f); q.strokeRoundRect(r, GuiCorners(8), 0.25f, STROKE::INSIDE, white);
		checkSane(q, "hairline 0.25");
		expectInk(q, per * 0.25, 0.03, "hairline 0.25");
		int amax = 0;
		for(size_t i = 0; i < q.size(); i++) amax = std::max(amax, (int)q.vertices()[i].c.a);
		CHECK(amax >= 120 && amax <= 135, "0.25px line should peak near 50%% alpha, got %d", amax);
	}
	{
		GuiMesh m(f); m.strokeArc({ 50, 50 }, 30, -90, 180, 6, rgba(255, 255, 255));
		checkSane(m, "arc");
		expectInk(m, kPi * 30.0 * 6.0, 0.01, "arc half circle");
		GuiMesh n(f); n.strokeArc({ 50, 50 }, 30, 90, -180, 6, rgba(255, 255, 255));
		checkSane(n, "arc negative sweep");
		expectInk(n, kPi * 30.0 * 6.0, 0.01, "arc negative sweep");
	}
	{
		GuiMesh m(f); std::vector<GuiPoint> t = { {0, 0}, {60, 0}, {30, 50} };
		m.strokePolygon(t, 3, STROKE::OUTSIDE, white);
		checkSane(m, "polygon stroke");
		CHECK(m.size() > 0, "polygon stroke empty");
	}
	{
		GuiMesh m(f); m.addLine({ 0, 0 }, { 100, 0 }, 4, white);
		checkSane(m, "line");
		expectInk(m, 100.0 * 4.0, 0.03, "line");
	}

	// Gradient: left end carries `from`, right end carries `to`.
	{
		GuiMesh m(0); GuiRect r = { 0, 0, 100, 40 };
		m.addRoundRect(r, GuiCorners(10), GuiFill::linear(rgba(255, 0, 0), rgba(0, 0, 255), 0));
		float minx = 1e9, maxx = -1e9; PixelColor cl = {0,0,0,0}, cr = {0,0,0,0};
		for(size_t i = 0; i < m.size(); i++)
		{
			const MeshVertex& v = m.vertices()[i];
			if(v.x < minx) { minx = v.x; cl = v.c; }
			if(v.x > maxx) { maxx = v.x; cr = v.c; }
		}
		CHECK(cl.r == 255 && cl.b == 0, "left end should be red, got %d,%d,%d", cl.r, cl.g, cl.b);
		CHECK(cr.r == 0 && cr.b == 255, "right end should be blue, got %d,%d,%d", cr.r, cr.g, cr.b);
		GuiMesh v(0); v.addRect(r, GuiFill::linear(rgba(0, 255, 0), rgba(0, 0, 0), 90));
		float miny = 1e9; PixelColor ct = {0,0,0,0};
		for(size_t i = 0; i < v.size(); i++) if(v.vertices()[i].y < miny) { miny = v.vertices()[i].y; ct = v.vertices()[i].c; }
		CHECK(ct.g == 255, "top of a 90 degree gradient should be the start color, got g=%d", ct.g);
	}

	// Feather: the fringe never extends past half the feather outside the edge.
	{
		GuiMesh m(2.0f); GuiRect r = { 20, 20, 100, 60 };
		m.addRoundRect(r, GuiCorners(12), white);
		float lo = 1e9, hi = -1e9;
		for(size_t i = 0; i < m.size(); i++) { lo = std::min(lo, m.vertices()[i].x); hi = std::max(hi, m.vertices()[i].x); }
		CHECK(lo >= 19.0f - 1e-3f && hi <= 121.0f + 1e-3f, "fringe extent %.3f..%.3f", lo, hi);
		GuiMesh none(0); none.addRoundRect(r, GuiCorners(12), white);
		for(size_t i = 0; i < none.size(); i++) CHECK(none.vertices()[i].c.a == 255, "no-feather mesh must be fully opaque");
	}

	// Shadow: peak alpha at the centre never exceeds the requested alpha, fades to 0.
	{
		GuiMesh m(1); GuiRect r = { 20, 20, 100, 60 };
		m.addShadow(r, GuiCorners(12), 16, 0, 0, 6, rgba(0, 0, 0, 160));
		checkSane(m, "shadow");
		int amax = 0, amin = 255;
		for(size_t i = 0; i < m.size(); i++) { amax = std::max(amax, (int)m.vertices()[i].c.a); amin = std::min(amin, (int)m.vertices()[i].c.a); }
		CHECK(amax == 160 && amin == 0, "shadow alpha range %d..%d", amin, amax);
	}

	// Degenerate input must never produce NaNs or crash.
	{
		GuiMesh m(1);
		GuiRect zero = { 5, 5, 0, 0 }, thin = { 5, 5, 0.3f, 50 }, tiny = { 5, 5, 2, 2 };
		m.addRoundRect(zero, GuiCorners(8), white);
		m.addRoundRect(thin, GuiCorners(8), white);
		m.addRoundRect(tiny, GuiCorners(50), white);
		m.addEllipse(tiny, white);
		m.strokeRoundRect(tiny, GuiCorners(50), 40, STROKE::INSIDE, white);
		m.strokeRoundRect(tiny, GuiCorners(-5), 40, STROKE::CENTRE, white);
		m.addShadow(tiny, GuiCorners(4), 100, -80, 0, 0, rgba(0, 0, 0, 200));
		m.addLine({ 1, 1 }, { 1, 1 }, 4, white);
		std::vector<GuiPoint> degenerate = { {0, 0}, {10, 0}, {20, 0} }; // collinear
		m.addPolygon(degenerate, white);
		m.strokeArc({ 0, 0 }, 10, 0, 0, 4, rgba(255, 255, 255, 255));
		checkSane(m, "degenerate");
	}

	// Budget: how many vertices does a typical card cost?
	{
		GuiMesh m(1); GuiRect r = { 0, 0, 289, 118 };
		m.addShadow(r, GuiCorners(12), 14, 0, 0, 4, rgba(0, 0, 0, 120));
		m.addRoundRect(r, GuiCorners(12), white);
		m.strokeRoundRect(r, GuiCorners(12), 2, STROKE::INSIDE, white);
		printf("card (shadow + fill + stroke): %zu vertices (%zu triangles)\n", m.size(), m.size() / 3);
		GuiMesh c(1); GuiRect q = { 0, 0, 26, 26 };
		c.addEllipse(q, white); c.strokeEllipse(q, 2, STROKE::INSIDE, white);
		printf("26px round button (fill + stroke): %zu vertices\n", c.size());
	}

	// Gallery: eyeball check of every building block.
	{
		PixelColor bg = rgba(18, 22, 29), panel = rgba(28, 35, 48), line = rgba(74, 90, 115), accent = rgba(255, 179, 71), teal = rgba(63, 208, 176);
		Image im(640, 480, bg);
		GuiMesh m(1);
		// two cards: normal and selected (glow + accent stroke)
		GuiRect c1 = { 24, 24, 180, 90 };
		m.addShadow(c1, GuiCorners(12), 14, 0, 0, 4, rgba(0, 0, 0, 150));
		m.addRoundRect(c1, GuiCorners(12), GuiFill::linear(rgba(36, 46, 64), panel, 90));
		m.strokeRoundRect(c1, GuiCorners(12), 2, STROKE::INSIDE, GuiFill::solid(rgba(43, 54, 71)));
		GuiRect c2 = { 224, 24, 180, 90 };
		m.addShadow(c2, GuiCorners(12), 16, 2, 0, 0, rgba(255, 179, 71, 110));
		m.addRoundRect(c2, GuiCorners(12), GuiFill::linear(rgba(36, 46, 64), panel, 90));
		m.strokeRoundRect(c2, GuiCorners(12), 2, STROKE::INSIDE, GuiFill::solid(accent));
		// capsule buttons: normal / focused (gradient)
		GuiRect b1 = { 424, 24, 190, 40 };
		m.addRoundRect(b1, GuiCorners(20), GuiFill::linear(rgba(74, 90, 115), rgba(43, 54, 71), 90));
		m.strokeRoundRect(b1, GuiCorners(20), 2, STROKE::INSIDE, GuiFill::solid(line));
		GuiRect b2 = { 424, 74, 190, 40 };
		m.addRoundRect(b2, GuiCorners(20), GuiFill::linear(rgba(255, 200, 110), rgba(255, 150, 40), 90));
		m.strokeRoundRect(b2, GuiCorners(20), 2, STROKE::INSIDE, GuiFill::solid(rgba(255, 220, 160)));
		// mixed corners (tab) and a battery-style bar
		GuiRect tab = { 24, 140, 120, 36 };
		m.addRoundRect(tab, GuiCorners(12, 12, 0, 0), GuiFill::solid(panel));
		m.strokeRoundRect(tab, GuiCorners(12, 12, 0, 0), 2, STROKE::INSIDE, GuiFill::solid(line));
		GuiRect bar = { 164, 150, 140, 12 };
		m.addRoundRect(bar, GuiCorners(6), GuiFill::solid(rgba(43, 54, 71)));
		GuiRect fillBar = { 164, 150, 98, 12 };
		m.addRoundRect(fillBar, GuiCorners(6), GuiFill::linear(teal, rgba(40, 150, 130), 0));
		// circles, rings, arcs
		for(int i = 0; i < 5; i++)
		{
			float s = 14.0f + i * 8.0f;
			GuiRect q = { 330.0f + i * 56.0f - s / 2, 158.0f - s / 2, s, s };
			m.addEllipse(q, GuiFill::solid(panel));
			m.strokeEllipse(q, 2, STROKE::INSIDE, GuiFill::solid(i == 2 ? teal : line));
		}
		m.strokeArc({ 60, 250 }, 30, -90, 270, 8, rgba(43, 54, 71));
		m.strokeArc({ 60, 250 }, 30, -90, 210, 8, teal);
		m.strokeArc({ 150, 250 }, 30, 0, 360, 1.0f, line);
		m.strokeArc({ 150, 250 }, 20, 0, 360, 0.5f, line);
		// polygons: triangle, chevron, line, hairline
		std::vector<GuiPoint> tri = { {220, 280}, {260, 280}, {240, 222} };
		m.addPolygon(tri, GuiFill::solid(accent));
		std::vector<GuiPoint> chev = { {290, 222}, {330, 222}, {350, 250}, {330, 278}, {290, 278}, {310, 250} };
		m.addPolygon(chev, GuiFill::linear(rgba(63, 208, 176), rgba(40, 120, 200), 0));
		m.strokePolygon(chev, 2, STROKE::OUTSIDE, GuiFill::solid(rgba(232, 237, 244)));
		m.addLine({ 380, 230 }, { 470, 270 }, 3, GuiFill::solid(rgba(232, 237, 244)));
		m.addLine({ 380, 270 }, { 470, 230 }, 1, GuiFill::solid(rgba(232, 237, 244)));
		m.addLine({ 380, 250 }, { 470, 250 }, 0.5f, GuiFill::solid(rgba(232, 237, 244)));
		// a controller built only from these blocks
		float ox = 40, oy = 310;
		GuiRect body = { ox + 10, oy + 34, 310, 120 };
		m.addShadow(body, GuiCorners(60), 18, 0, 0, 6, rgba(0, 0, 0, 160));
		m.addRoundRect(body, GuiCorners(60), GuiFill::linear(rgba(52, 64, 86), rgba(30, 38, 52), 90));
		m.strokeRoundRect(body, GuiCorners(60), 2, STROKE::INSIDE, GuiFill::solid(line));
		m.addRoundRect({ ox + 30, oy + 10, 56, 22 }, GuiCorners(11), GuiFill::solid(line));
		m.addRoundRect({ ox + 244, oy + 10, 56, 22 }, GuiCorners(11), GuiFill::solid(line));
		GuiRect st = { ox + 58, oy + 58, 44, 44 };
		m.addEllipse(st, GuiFill::solid(rgba(14, 18, 24))); m.strokeEllipse(st, 2, STROKE::INSIDE, GuiFill::solid(line));
		m.addEllipse({ ox + 72, oy + 70, 16, 16 }, GuiFill::solid(teal));
		m.addRoundRect({ ox + 66, oy + 108, 12, 36 }, GuiCorners(3), GuiFill::solid(rgba(14, 18, 24)));  // d-pad: two bars
		m.addRoundRect({ ox + 54, oy + 120, 36, 12 }, GuiCorners(3), GuiFill::solid(rgba(14, 18, 24)));
		const float bx[4] = { 250, 224, 276, 250 }, by[4] = { 66, 92, 92, 118 };
		for(int i = 0; i < 4; i++)
		{
			GuiRect q = { ox + bx[i] - 13, oy + by[i] - 13, 26, 26 };
			m.addEllipse(q, GuiFill::solid(i == 2 ? teal : rgba(35, 45, 61)));
			m.strokeEllipse(q, 2, STROKE::INSIDE, GuiFill::solid(i == 2 ? teal : line));
		}
		drawTriangles(im, m.vertices());
		CHECK(writePng(im, "out/gallery.png", 1), "could not write out/gallery.png (mkdir test/out)");
		// 6x zoom on the selected card's corner to inspect the anti-aliasing
		Image zoom(110, 70, bg);
		drawTriangles(zoom, m.vertices(), -214, -14);
		CHECK(writePng(zoom, "out/zoom_card_corner.png", 6), "could not write zoom image");
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
