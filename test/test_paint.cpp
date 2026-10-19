// Host-side tests for GuiPaint, GuiShapeStyle and GuiTheme. Run with `make -C test`.
#include "../source/libgui/GuiTheme.h"
#include <cstdio>
#include <cmath>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

static PixelColor rgba(int r, int g, int b, int a = 255) { PixelColor c = { (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a }; return c; }
static bool near8(int a, int b, int tol = 1) { return abs(a - b) <= tol; }

int main()
{
	const PixelColor red = rgba(255, 0, 0), blue = rgba(0, 0, 255), black = rgba(0, 0, 0, 160);

	// equality compares only what is visible
	{
		GuiPaint a = GuiPaint().color(red).stroke(2, blue);
		GuiPaint b = GuiPaint().color(red).stroke(2, blue);
		CHECK(a == b, "identical paints must be equal");
		CHECK(a != GuiPaint().color(blue).stroke(2, blue), "different fill must differ");
		CHECK(a != GuiPaint().color(red).stroke(3, blue), "different stroke width must differ");
		GuiPaint c = GuiPaint().color(red);
		GuiPaint d = GuiPaint().color(red).shadow(10, rgba(0, 0, 0, 0)); // alpha 0 = no shadow
		CHECK(c == d, "a fully transparent shadow is no shadow");
		GuiPaint e; e.stroke(0, blue); // zero width = no stroke, whatever its color
		CHECK(e == GuiPaint(), "a zero-width stroke is no stroke");
	}

	// mix: exact at the ends, halfway in the middle
	{
		GuiPaint a = GuiPaint().color(red).stroke(2, red);
		GuiPaint b = GuiPaint().color(blue).stroke(6, blue);
		CHECK(GuiPaint::mix(a, b, 0.0f) == a, "mix at 0 is a");
		CHECK(GuiPaint::mix(a, b, 1.0f) == b, "mix at 1 is b");
		CHECK(GuiPaint::mix(a, b, -3.0f) == a && GuiPaint::mix(a, b, 9.0f) == b, "t is clamped");
		GuiPaint m = GuiPaint::mix(a, b, 0.5f);
		CHECK(m.hasFill && !m.fill.gradient, "solid + solid stays solid");
		CHECK(near8(m.fill.from.r, 128) && near8(m.fill.from.b, 128), "half way fill is %d,%d,%d", m.fill.from.r, m.fill.from.g, m.fill.from.b);
		CHECK(fabsf(m.strokeWidth - 4.0f) < 1e-4f, "stroke width %.3f", m.strokeWidth);
	}

	// parts present on one side only fade rather than pop
	{
		GuiPaint plain = GuiPaint().color(red);
		GuiPaint lit = GuiPaint().color(red).stroke(2, blue).glow(12, rgba(255, 150, 40, 110), 2);
		GuiPaint m = GuiPaint::mix(plain, lit, 0.5f);
		CHECK(near8(m.strokeWidth * 10, 10, 1), "stroke grows from zero: %.3f", m.strokeWidth);
		CHECK(m.strokeFill.from.b == 255, "a stroke growing in uses its own color from the start");
		CHECK(near8(m.shadowColor.a, 55, 1), "shadow fades in: alpha %d", m.shadowColor.a);
		CHECK(m.shadowColor.r == 255 && m.shadowColor.g == 150, "shadow keeps its own color while fading");
		GuiPaint out = GuiPaint::mix(lit, plain, 0.5f);
		CHECK(near8(out.shadowColor.a, 55, 1) && out.hasFill, "fading out is the mirror image");
		GuiPaint nofill = GuiPaint().stroke(2, blue);
		GuiPaint fillIn = GuiPaint::mix(nofill, GuiPaint().color(red), 0.5f);
		CHECK(fillIn.hasFill && near8(fillIn.fill.from.a, 128), "a fill fades in from transparent: alpha %d", fillIn.fill.from.a);
	}

	// solid <-> gradient blends through gradient form
	{
		GuiPaint s = GuiPaint().color(red);
		GuiPaint g = GuiPaint().gradient(blue, black, 90);
		GuiPaint m = GuiPaint::mix(s, g, 0.5f);
		CHECK(m.fill.gradient, "mixing with a gradient yields a gradient");
		CHECK(near8(m.fill.from.r, 128), "gradient start r %d", m.fill.from.r);
		CHECK(fabsf(m.fill.angle - 90.0f) < 1e-3f, "gradient angle %.2f", m.fill.angle);
	}

	// style fallbacks
	{
		GuiPaint rest = GuiPaint().color(red), lit = GuiPaint().color(blue);
		GuiShapeStyle only(rest);
		CHECK(only.forState(SHAPE_STATE::FOCUSED) == rest && only.forState(SHAPE_STATE::PRESSED) == rest && only.forState(SHAPE_STATE::DISABLED) == rest, "unset states fall back to normal");
		GuiShapeStyle two = GuiShapeStyle(rest).onFocus(lit);
		CHECK(two.forState(SHAPE_STATE::FOCUSED) == lit, "focused paint");
		CHECK(two.forState(SHAPE_STATE::PRESSED) == lit, "an unset pressed state follows focused (a press happens while focused)");
		CHECK(two.forState(SHAPE_STATE::DISABLED) == rest, "disabled falls back to normal, not focused");
		GuiShapeStyle off = GuiShapeStyle(rest).onFocus(lit).onDisable(GuiPaint().color(black));
		CHECK(off.forState(SHAPE_STATE::DISABLED) == GuiPaint().color(black), "disabled paint");
	}

	// painted shapes: each part contributes, in draw order shadow -> fill -> stroke
	{
		GuiRect r = { 0, 0, 120, 50 };
		GuiMesh fillOnly(1), withStroke(1), all(1), none(1);
		GuiAddPaintedShape(none, SHAPE::RECTANGLE, r, GuiCorners(10), GuiPaint());
		CHECK(none.empty(), "an empty paint draws nothing");
		GuiAddPaintedShape(fillOnly, SHAPE::RECTANGLE, r, GuiCorners(10), GuiPaint().color(red));
		GuiAddPaintedShape(withStroke, SHAPE::RECTANGLE, r, GuiCorners(10), GuiPaint().color(red).stroke(2, blue));
		GuiAddPaintedShape(all, SHAPE::RECTANGLE, r, GuiCorners(10), GuiPaint().color(red).stroke(2, blue).shadow(10, black, 0, 4));
		CHECK(fillOnly.size() > 0 && withStroke.size() > fillOnly.size() && all.size() > withStroke.size(), "%zu < %zu < %zu", fillOnly.size(), withStroke.size(), all.size());
		CHECK(all.vertices()[0].c.r == 0 && all.vertices()[0].c.a <= 160, "the shadow comes first");
		GuiMesh ell(1);
		GuiRect sq = { 0, 0, 40, 40 };
		GuiAddPaintedShape(ell, SHAPE::ELLIPSE, sq, GuiCorners(0), GuiPaint().color(red).stroke(2, blue).glow(8, black));
		CHECK(ell.size() % 3 == 0 && ell.size() > 0, "ellipse paint");
	}

	// every theme style produces geometry in every state, and states differ
	{
		GuiTheme theme;
		GuiShapeStyle styles[4] = { theme.panelStyle(), theme.cardStyle(), theme.buttonStyle(), theme.chipStyle() };
		const char* names[4] = { "panel", "card", "button", "chip" };
		GuiRect r = { 0, 0, 160, 44 };
		for(int i = 0; i < 4; i++)
		{
			SHAPE_STATE st[4] = { SHAPE_STATE::NORMAL, SHAPE_STATE::FOCUSED, SHAPE_STATE::PRESSED, SHAPE_STATE::DISABLED };
			for(int k = 0; k < 4; k++)
			{
				GuiMesh m(1);
				GuiAddPaintedShape(m, SHAPE::RECTANGLE, r, GuiCorners(theme.radius), styles[i].forState(st[k]));
				CHECK(m.size() > 0 && m.size() % 3 == 0, "%s state %d produced %zu vertices", names[i], k, m.size());
			}
		}
		CHECK(theme.buttonStyle().forState(SHAPE_STATE::FOCUSED) != theme.buttonStyle().forState(SHAPE_STATE::NORMAL), "a button looks different when focused");
		CHECK(theme.cardStyle().forState(SHAPE_STATE::DISABLED).fill.from.a < 255, "a disabled card is see-through");
		CHECK(&GuiTheme::current() == &GuiTheme::current(), "one shared current theme");
		GuiTheme::current().accent = rgba(10, 200, 10);
		CHECK(GuiTheme::current().buttonStyle().forState(SHAPE_STATE::FOCUSED).fill.to.g == 200, "styles follow the theme");
	}

	// the shared color mix, and styles that follow the theme (no baked-in colors)
	{
		PixelColor a = { 0, 100, 200, 255 }, b = { 100, 200, 0, 55 };
		PixelColor m = GuiMixColor(a, b, 0.5f);
		CHECK(m.r == 50 && m.g == 150 && m.b == 100 && m.a == 155, "mix: %d,%d,%d,%d", m.r, m.g, m.b, m.a);
		CHECK(GuiMixColor(a, b, 0.0f).g == 100 && GuiMixColor(a, b, 1.0f).g == 200 && GuiMixColor(a, b, 9.0f).g == 200, "mix ends and clamp");

		GuiTheme gray;
		gray.background = rgba(30, 30, 30); gray.surface = rgba(46, 46, 46); gray.surfaceRaised = rgba(62, 62, 62);
		gray.outline = rgba(84, 84, 84); gray.outlineStrong = rgba(120, 120, 120);
		gray.highlight = GuiMixColor(gray.surfaceRaised, gray.outlineStrong, 0.65f);
		const PixelColor row = gray.rowStyle().forState(SHAPE_STATE::FOCUSED).fill.from;
		CHECK(row.r == row.g && row.g == row.b, "a gray theme gives a gray focus bar: %d,%d,%d", row.r, row.g, row.b);
		const PixelColor bar = gray.barStyle().forState(SHAPE_STATE::NORMAL).fill.from;
		CHECK(bar.r == bar.g && bar.g == bar.b && bar.r < gray.background.r, "and a gray, darker bar: %d,%d,%d", bar.r, bar.g, bar.b);
	}

	// highlight, mark and shadow: new theme fields whose defaults leave the look as it was
	{
		GuiTheme t;
		const PixelColor derived = GuiMixColor(t.surfaceRaised, t.outlineStrong, 0.65f);
		PixelColor row = t.rowStyle().forState(SHAPE_STATE::FOCUSED).fill.from;
		CHECK(t.highlight.a == 255 && t.highlight.r == derived.r && t.highlight.g == derived.g && t.highlight.b == derived.b, "highlight defaults to the mix the row bar always used");
		CHECK(row.r == derived.r && row.g == derived.g && row.b == derived.b, "so the default row bar is unchanged");
		t.highlight = rgba(1, 2, 3);
		row = t.rowStyle().forState(SHAPE_STATE::FOCUSED).fill.from;
		CHECK(row.r == 1 && row.g == 2 && row.b == 3, "a set highlight is the row bar: %d,%d,%d", row.r, row.g, row.b);
		CHECK(t.mark.r == 255 && t.mark.g == 150 && t.mark.b == 40, "mark defaults to the default accent");
		GuiTheme u;
		CHECK(u.cardStyle().forState(SHAPE_STATE::NORMAL).shadowColor.a == 150, "the card's shadow keeps its old strength");
		u.shadow = rgba(0, 0, 0, 64);
		CHECK(u.cardStyle().forState(SHAPE_STATE::NORMAL).shadowColor.a == 64, "the card's shadow follows the theme");
	}

	// triangles: fill only, one winding, the apex on the middle of a side, half of the rectangle covered
	{
		const GuiRect r = { 10, 20, 40, 30 };
		struct Case { SHAPE shape; float ax, ay; const char * name; } cases[4] = {
			{ SHAPE::TRIANGLE_UP, 30, 20, "up" }, { SHAPE::TRIANGLE_DOWN, 30, 50, "down" },
			{ SHAPE::TRIANGLE_LEFT, 10, 35, "left" }, { SHAPE::TRIANGLE_RIGHT, 50, 35, "right" } };
		const GuiPaint plain = GuiPaint().color(red);
		const GuiPaint busy = GuiPaint().color(red).stroke(3, blue).shadow(10, black, 2, 2);
		for(int i = 0; i < 4; i++)
		{
			GuiMesh m(1);
			GuiAddPaintedShape(m, cases[i].shape, r, GuiCorners(8.0f), plain);
			GuiMesh b(1);
			GuiAddPaintedShape(b, cases[i].shape, r, GuiCorners(8.0f), busy);
			const std::vector<MeshVertex>& v = m.vertices();
			double ink = 0;
			int wrong = 0;
			float nearest = 1e9f, minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
			for(size_t k = 0; k + 2 < v.size(); k += 3)
			{
				double s = (v[k+1].x - v[k].x) * (v[k+2].y - v[k].y) - (v[k+2].x - v[k].x) * (v[k+1].y - v[k].y);
				if(s < -1e-3) wrong++;
				ink += 0.5 * fabs(s) * (v[k].c.a + v[k+1].c.a + v[k+2].c.a) / (3.0 * 255.0);
			}
			for(size_t k = 0; k < v.size(); k++)
			{
				nearest = fminf(nearest, hypotf(v[k].x - cases[i].ax, v[k].y - cases[i].ay));
				minX = fminf(minX, v[k].x); maxX = fmaxf(maxX, v[k].x); minY = fminf(minY, v[k].y); maxY = fmaxf(maxY, v[k].y);
			}
			CHECK(v.size() > 0 && v.size() % 3 == 0, "triangle %s: %zu vertices", cases[i].name, v.size());
			CHECK(wrong == 0, "triangle %s: %d triangles wound the wrong way", cases[i].name, wrong);
			CHECK(fabs(ink - 0.5 * r.w * r.h) < 0.03 * 0.5 * r.w * r.h, "triangle %s covers %.1f of %.1f", cases[i].name, ink, 0.5 * r.w * r.h);
			CHECK(nearest < 1.5f, "triangle %s: the apex is %.2f from the middle of its side", cases[i].name, nearest);
			CHECK(minX > r.x - 1.5f && maxX < r.x + r.w + 1.5f && minY > r.y - 1.5f && maxY < r.y + r.h + 1.5f, "triangle %s stays in its rectangle", cases[i].name);
			CHECK(b.vertices().size() == v.size(), "triangle %s: no outline or shadow is drawn (%zu vs %zu vertices)", cases[i].name, b.vertices().size(), v.size());
		}
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
