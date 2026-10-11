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

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
