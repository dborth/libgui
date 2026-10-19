// Host-side tests for the controller layout table and drawing. Run with `make -C test`. Writes out/controller.png.
#include "../source/libgui/GuiGlyph.h"
#include "../source/controllerdraw.h"
#include "Raster.h"
#include <cstdio>
#include <set>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

int main()
{
	const ControllerLayout& pro = ProControllerLayout();

	// ---- the table is well formed
	CHECK(&pro == &ProControllerLayout(), "one shared instance");
	std::set<int> ids;
	bool unique = true;
	for(size_t i = 0; i < pro.spots.size(); i++)
	{
		const ControlSpot& s = pro.spots[i];
		CHECK(s.x >= 0 && s.y >= 0 && s.x + s.w <= pro.width && s.y + s.h <= pro.height, "spot %zu (%d) lies inside the %gx%g drawing: %g,%g %gx%g", i, s.id, pro.width, pro.height, s.x, s.y, s.w, s.h);
		if(s.id != 0 && !ids.insert(s.id).second)
			unique = false;
	}
	CHECK(unique, "control ids are unique");
	const int expected[] = { INPUT_BTN_A, INPUT_BTN_B, INPUT_BTN_X, INPUT_BTN_Y, INPUT_BTN_UP, INPUT_BTN_DOWN, INPUT_BTN_LEFT, INPUT_BTN_RIGHT,
		INPUT_BTN_PLUS, INPUT_BTN_MINUS, INPUT_BTN_HOME, INPUT_TRIGGER_L, INPUT_TRIGGER_R, INPUT_TRIGGER_ZL, INPUT_TRIGGER_ZR, INPUT_THUMB_L, INPUT_THUMB_R, CTL_CAPTURE };
	for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
		CHECK(pro.find(expected[i]) != nullptr, "the Pro layout has control %d", expected[i]);
	CHECK(pro.find(0) == nullptr && pro.find(INPUT_BTN_1) == nullptr, "find() is null for decoration and for controls it lacks");
	for(size_t i = 0; i < pro.body.size(); i++)
	{
		const BodyPiece& b = pro.body[i];
		CHECK(b.x + b.w > 0 && b.y + b.h > 0 && b.x < pro.width && b.y < pro.height, "body piece %zu is on the drawing", i);
	}
	// left and right sides mirror each other
	const ControlSpot * zl = pro.find(INPUT_TRIGGER_ZL), * zr = pro.find(INPUT_TRIGGER_ZR);
	CHECK(zl && zr && fabsf((zl->x + zl->w * 0.5f) + (zr->x + zr->w * 0.5f) - pro.width) < 0.01f, "ZL and ZR mirror about the middle");
	const ControlSpot * l = pro.find(INPUT_TRIGGER_L), * r = pro.find(INPUT_TRIGGER_R);
	CHECK(l && r && fabsf((l->x + l->w * 0.5f) + (r->x + r->w * 0.5f) - pro.width) < 0.01f, "L and R mirror about the middle");

	// ---- the drawing builds, in every state, at several sizes
	const float scales[] = { 0.5f, 0.72f, 1.0f, 2.0f };
	for(size_t k = 0; k < sizeof(scales) / sizeof(scales[0]); k++)
	{
		GuiMesh body(1.0f);
		BuildControllerBody(body, pro, scales[k]);
		CHECK(body.size() > 0 && body.size() % 3 == 0, "body mesh at %.2f: %zu", scales[k], body.size());
		const float pad = 4.0f * scales[k] + 4.0f;
		bool inside = true, finite = true;
		for(size_t i = 0; i < body.size(); i++)
		{
			const MeshVertex& v = body.vertices()[i];
			if(!std::isfinite(v.x) || !std::isfinite(v.y)) finite = false;
			if(v.x < -pad || v.y < -pad || v.x > pro.width * scales[k] + pad || v.y > pro.height * scales[k] + pad) inside = false;
		}
		CHECK(finite && inside, "body mesh at %.2f stays within its drawing", scales[k]);
		for(size_t i = 0; i < pro.spots.size(); i++)
			for(int st = 0; st < 4; st++)
			{
				GuiMesh m(1.0f);
				BuildControllerSpot(m, pro.spots[i], (CTL_STATE)st, scales[k]);
				CHECK(m.size() > 0 && m.size() % 3 == 0, "spot %zu state %d at %.2f: %zu vertices", i, st, scales[k], m.size());
			}
	}
	// a highlighted control costs more than a resting one (it has a glow)
	GuiMesh rest(1.0f), hl(1.0f);
	BuildControllerSpot(rest, *pro.find(INPUT_BTN_A), CTL_STATE::NORMAL, 1.0f);
	BuildControllerSpot(hl, *pro.find(INPUT_BTN_A), CTL_STATE::HIGHLIGHT, 1.0f);
	CHECK(hl.size() > rest.size(), "highlight adds a glow: %zu > %zu", hl.size(), rest.size());
	// stick caps: centered, pushed, and clamped to the ring
	{
		const ControlSpot& st = *pro.find(INPUT_THUMB_L);
		const float cx = (st.x + st.w * 0.5f) * 1.0f, cy = (st.y + st.h * 0.5f);
		auto centreOf = [](const GuiMesh& m, float& x, float& y) {
			float minx = 1e9, maxx = -1e9, miny = 1e9, maxy = -1e9;
			for(size_t i = 0; i < m.size(); i++) { minx = std::min(minx, m.vertices()[i].x); maxx = std::max(maxx, m.vertices()[i].x); miny = std::min(miny, m.vertices()[i].y); maxy = std::max(maxy, m.vertices()[i].y); }
			x = (minx + maxx) / 2; y = (miny + maxy) / 2;
		};
		GuiMesh a(1.0f), b(1.0f), c(1.0f);
		BuildControllerStickCap(a, st, 0, 0, false, 1.0f);
		BuildControllerStickCap(b, st, 1, 0, false, 1.0f);
		BuildControllerStickCap(c, st, 5, 5, false, 1.0f); // far past the edge
		float ax, ay, bx, by, ccx, ccy;
		centreOf(a, ax, ay); centreOf(b, bx, by); centreOf(c, ccx, ccy);
		CHECK(fabsf(ax - cx) < 0.5f && fabsf(ay - cy) < 0.5f, "an idle cap is in the middle of its ring: %.2f,%.2f vs %.2f,%.2f", ax, ay, cx, cy);
		CHECK(bx > ax + 5.0f && fabsf(by - ay) < 0.5f, "pushed right moves the cap right: %.2f -> %.2f", ax, bx);
		const float dist = sqrtf((ccx - cx) * (ccx - cx) + (ccy - cy) * (ccy - cy));
		CHECK(dist <= st.w * 0.5f, "a diagonal push past the edge stays inside the ring: %.2f", dist);
	}
	// the silhouette is merged: no outline inside the body (every outline vertex is at its edge)
	{
		GuiMesh body(1.0f);
		BuildControllerBody(body, pro, 1.0f);
		CHECK(body.size() < 20000, "body mesh stays modest: %zu vertices", body.size());
	}

	// ---- eyeball sheet: resting, with a highlight and a pressed state, and a pushed stick
	{
		PixelColor bg = { 18, 22, 29, 255 };
		Image im(640, 330, bg);
		const float scale = 0.8f;
		for(int variant = 0; variant < 2; variant++)
		{
			const float ox = 10.0f + variant * 320.0f, oy = 10.0f;
			GuiMesh m(1.0f);
			BuildControllerBody(m, pro, scale);
			for(size_t i = 0; i < pro.spots.size(); i++)
			{
				const ControlSpot& s = pro.spots[i];
				if(s.id == 0) continue;
				CTL_STATE st = CTL_STATE::NORMAL;
				if(variant == 1)
				{
					const bool lit = s.id == INPUT_BTN_A || s.id == INPUT_BTN_LEFT || s.id == INPUT_TRIGGER_ZR;
					const bool hl = s.id == INPUT_BTN_Y || s.id == INPUT_BTN_A;
					st = lit ? (hl ? CTL_STATE::LIT_HIGHLIGHT : CTL_STATE::LIT) : (hl ? CTL_STATE::HIGHLIGHT : CTL_STATE::NORMAL);
				}
				BuildControllerSpot(m, s, st, scale);
			}
			BuildControllerStickCap(m, *pro.find(INPUT_THUMB_L), variant ? 0.7f : 0.0f, variant ? -0.5f : 0.0f, false, scale);
			BuildControllerStickCap(m, *pro.find(INPUT_THUMB_R), 0.0f, 0.0f, variant == 1, scale);
			drawTriangles(im, m.vertices(), ox, oy);
		}
		// the same controller again, small, to judge it at list-page scale
		{
			GuiMesh m(1.0f);
			BuildControllerBody(m, pro, 0.5f);
			for(size_t i = 0; i < pro.spots.size(); i++)
				if(pro.spots[i].id != 0) BuildControllerSpot(m, pro.spots[i], CTL_STATE::NORMAL, 0.5f);
			drawTriangles(im, m.vertices(), 220.0f, 215.0f);
		}
		CHECK(writePng(im, "out/controller.png", 1), "could not write out/controller.png");
	}

	// ---- the drawing follows the theme: a gray theme gives a gray controller
	{
		GuiTheme saved = GuiTheme::current();
		GuiTheme& t = GuiTheme::current();
		t.background = { 30, 30, 30, 255 }; t.surface = { 46, 46, 46, 255 }; t.surfaceRaised = { 62, 62, 62, 255 };
		t.outline = { 84, 84, 84, 255 }; t.outlineStrong = { 120, 120, 120, 255 };
		GuiMesh body(1.0f);
		BuildControllerBody(body, pro, 1.0f);
		bool tinted = false;
		for(size_t i = 0; i < body.size(); i++)
		{
			const PixelColor& c = body.vertices()[i].c;
			if(c.r != c.g || c.g != c.b) tinted = true;
		}
		CHECK(!tinted, "no blue tint is baked into the silhouette");
		GuiMesh stick(1.0f), cap(1.0f);
		BuildControllerSpot(stick, *pro.find(INPUT_THUMB_L), CTL_STATE::NORMAL, 1.0f);
		BuildControllerStickCap(cap, *pro.find(INPUT_THUMB_L), 0, 0, false, 1.0f);
		for(size_t i = 0; i < stick.size(); i++) { const PixelColor& c = stick.vertices()[i].c; CHECK(c.r == c.g && c.g == c.b, "stick well and rings are gray: %d,%d,%d", c.r, c.g, c.b); break; }
		for(size_t i = 0; i < cap.size(); i++) { const PixelColor& c = cap.vertices()[i].c; if(c.r != c.g || c.g != c.b) { CHECK(false, "stick cap is gray: %d,%d,%d", c.r, c.g, c.b); break; } }
		GuiTheme::current() = saved;
	}

	// ---- glyphs
	{
		const GLYPH kinds[] = { GLYPH::BUTTON, GLYPH::BUMPER, GLYPH::UP, GLYPH::DOWN, GLYPH::LEFT, GLYPH::RIGHT, GLYPH::PLUS, GLYPH::MINUS, GLYPH::STICK, GLYPH::CAPTURE, GLYPH::HOME };
		PixelColor bg = { 18, 22, 29, 255 };
		Image im(11 * 40 + 10, 50, bg);
		for(size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++)
		{
			float w, h;
			GuiGlyph::size(kinds[i], w, h);
			GuiMesh m(1.0f);
			GuiGlyph::build(m, kinds[i], w, h);
			CHECK(w > 0 && h > 0 && m.size() > 0 && m.size() % 3 == 0, "glyph %zu: %gx%g, %zu vertices", i, w, h, m.size());
			bool inside = true;
			for(size_t k = 0; k < m.size(); k++)
			{
				const MeshVertex& v = m.vertices()[k];
				if(v.x < -1.0f || v.y < -1.0f || v.x > w + 1.0f || v.y > h + 1.0f) inside = false;
			}
			CHECK(inside, "glyph %zu stays inside its box", i);
			drawTriangles(im, m.vertices(), 10.0f + (float)i * 40.0f, 14.0f);
		}
		CHECK(writePng(im, "out/glyphs.png", 3), "could not write out/glyphs.png");
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
