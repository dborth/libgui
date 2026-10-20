// Host test of GuiPopoverPlacement: where a popover goes and which way its tail points. Pure arithmetic: needs no
// platform and no FreeType.
#include "../source/libgui/GuiPopoverPlacement.h"
#include <cstdio>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

int main()
{
	const int SW = 640, SH = 480;
	const GuiPopoverInsets ins(84, 100, 44);		// a top bar, a bottom bar and a side margin
	const int gap = 6;

	CHECK(GuiPopoverMaxHeight(SH, ins) == 480 - 84 - 100 - 2 * gap, "the band between the bars: %d", GuiPopoverMaxHeight(SH, ins));

	// below its anchor when it fits, lined up with the nearer edge, inside the margins
	GuiPopoverPosition p = GuiPopoverPlace(GuiPopoverAnchor(400, 60, 150, 32), 280, 200, SW, SH, ins);
	CHECK(p.y == 60 + 32 + gap, "under the anchor: %d", p.y);
	CHECK(p.x == 400 + 150 - 280, "right half: right edges line up: %d", p.x);
	p = GuiPopoverPlace(GuiPopoverAnchor(60, 40, 100, 32), 280, 200, SW, SH, ins);
	CHECK(p.x == 60, "left half: left edges line up: %d", p.x);
	p = GuiPopoverPlace(GuiPopoverAnchor(60, 40, 100, 32, GuiPopoverAnchor::RIGHT_MARGIN), 280, 200, SW, SH, ins);
	CHECK(p.x == SW - 44 - 280, "against the right margin: %d", p.x);
	p = GuiPopoverPlace(GuiPopoverAnchor(300, 40, 100, 32, GuiPopoverAnchor::LEFT_MARGIN), 280, 200, SW, SH, ins);
	CHECK(p.x == 44, "against the left margin: %d", p.x);
	p = GuiPopoverPlace(GuiPopoverAnchor(560, 400, 100, 32), 280, 200, SW, SH, ins);
	CHECK(p.x == SW - 44 - 280, "kept inside the margin: %d", p.x);
	CHECK(p.y == 400 - 200 - gap, "no room below a footer chip: it comes out of the top of it: %d", p.y);
	p = GuiPopoverPlace(GuiPopoverAnchor(100, 200, 120, 30), 280, 280, SW, SH, ins);
	CHECK(p.y >= 84 && p.y + 280 <= SH - 100, "a row in the middle of the page: kept in the band: %d", p.y);
	p = GuiPopoverPlace(GuiPopoverAnchor(), 280, 200, SW, SH, ins);
	CHECK(p.x == 180 && p.y == 140, "no anchor: centered: %d,%d", p.x, p.y);

	// the tail points at the anchor: up from a panel under it, down from one over it, sideways from one beside it
	{
		const GuiPopoverAnchor a(400, 20, 150, 40);
		const GuiPopoverPosition under = GuiPopoverPlace(a, 280, 200, SW, SH, ins);
		GuiPopoverTail t = GuiPopoverTailAt(a, under, 280, 200);
		CHECK(t.shown && t.direction == POPOVER_TAIL::UP && t.y + t.h == under.y + 1, "under: up, ending on the panel's top edge");
		CHECK(t.x >= under.x && t.x + t.w <= under.x + 280, "and inside the panel's width");
		const GuiPopoverAnchor b(400, 420, 150, 40);
		const GuiPopoverPosition over = GuiPopoverPlace(b, 280, 200, SW, SH, ins);
		t = GuiPopoverTailAt(b, over, 280, 200);
		CHECK(t.shown && t.direction == POPOVER_TAIL::DOWN && t.y == over.y + 200 - 1, "over: down, starting on the panel's bottom edge");
		const GuiPopoverAnchor row(60, 200, 170, 30, GuiPopoverAnchor::RIGHT_MARGIN);
		const GuiPopoverPosition beside = GuiPopoverPlace(row, 380, 270, SW, SH, ins);
		t = GuiPopoverTailAt(row, beside, 380, 270);
		CHECK(t.shown && t.direction == POPOVER_TAIL::LEFT, "beside it on the right: left");
		CHECK(t.y >= beside.y && t.y + t.h <= beside.y + 270, "and inside the panel's height");
		const GuiPopoverAnchor right(560, 200, 60, 30, GuiPopoverAnchor::LEFT_MARGIN);
		const GuiPopoverPosition onLeft = GuiPopoverPlace(right, 380, 270, SW, SH, ins);
		t = GuiPopoverTailAt(right, onLeft, 380, 270);
		CHECK(t.shown && t.direction == POPOVER_TAIL::RIGHT && t.x == onLeft.x + 380 - 1, "beside it on the left: right");
		const GuiPopoverAnchor covered(300, 200, 170, 30, GuiPopoverAnchor::RIGHT_MARGIN);
		CHECK(!GuiPopoverTailAt(covered, GuiPopoverPlace(covered, 380, 270, SW, SH, ins), 380, 270).shown, "the panel covers the anchor: no tail");
		CHECK(!GuiPopoverTailAt(GuiPopoverAnchor(), under, 280, 200).shown, "no anchor: no tail");
	}

	// whatever the anchor: the panel keeps inside the side margins, and inside the band when it can fit in it
	{
		const int widths[] = { 200, 280, 380 }, heights[] = { 120, 200, 270, 320 };
		const GuiPopoverAnchor::Edge edges[] = { GuiPopoverAnchor::AUTO, GuiPopoverAnchor::LEFT_MARGIN, GuiPopoverAnchor::RIGHT_MARGIN };
		int bad = 0, badTail = 0, n = 0;
		for(int ax = 0; ax <= 600; ax += 40)
			for(int ay = 0; ay <= 450; ay += 30)
				for(int w : widths) for(int h : heights) for(GuiPopoverAnchor::Edge e : edges)
				{
					const GuiPopoverAnchor a(ax, ay, 100, 32, e);
					const GuiPopoverPosition q = GuiPopoverPlace(a, w, h, SW, SH, ins);
					n++;
					if(q.x < ins.side || q.x + w > SW - ins.side) bad++;
					// A chip in a bar gets a panel that comes right out of it, so the band only binds an anchor inside the band
					const bool fits = h <= GuiPopoverMaxHeight(SH, ins);
					const bool anchorInBand = ay >= ins.top && ay + 32 <= SH - ins.bottom;
					if(anchorInBand && fits && (q.y < ins.top || q.y + h > SH - ins.bottom)) bad++;
					if(ay + 32 + gap + h <= SH - ins.bottom - gap && q.y != ay + 32 + gap) bad++;	// room below: straight under it
					const GuiPopoverTail t = GuiPopoverTailAt(a, q, w, h);
					if(t.shown && (t.w <= 0 || t.h <= 0)) badTail++;
					if(t.shown && (t.direction == POPOVER_TAIL::UP || t.direction == POPOVER_TAIL::DOWN) && (t.x < q.x || t.x + t.w > q.x + w)) badTail++;
					if(t.shown && (t.direction == POPOVER_TAIL::LEFT || t.direction == POPOVER_TAIL::RIGHT) && (t.y < q.y || t.y + t.h > q.y + h)) badTail++;
				}
		CHECK(bad == 0, "%d of %d placements left the margins or the band", bad, n);
		CHECK(badTail == 0, "%d of %d tails were empty or off their panel's edge", badTail, n);
	}

	// the default insets keep nothing clear
	{
		const GuiPopoverInsets none;
		CHECK(none.top == 0 && none.bottom == 0 && none.side == 0, "default insets are none");
		CHECK(GuiPopoverMaxHeight(SH, none) == SH - 2 * gap, "no bars: the whole screen less the gaps");
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
