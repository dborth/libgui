// Host-side integration test: real GuiRow/GuiColumn/GuiGrid/GuiOverlay/GuiWindow/GuiElement
// with stand-in leaf elements. Run with `make -C test`.
#include "../source/libgui/Gui.h"
#include <cstdio>
#include <cmath>

Platform * platform = nullptr; // never used by the code under test

// GuiWindow::update() reads the controller; the tests drive selection directly instead
bool InputController::up() const { return false; }
bool InputController::down() const { return false; }
bool InputController::left() const { return false; }
bool InputController::right() const { return false; }
bool InputController::isSecondaryPressed() const { return false; }

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

// A leaf with a size and nothing to draw. `selectable` makes it behave like a button for navigation.
class Box : public GuiElement
{
	public:
		Box(int w, int h, bool sel = false) { width = w; height = h; selectable = sel; }
		void draw() override {}
};

// Mimics GuiText: reports a measured size, and a size set by layout would be wrong for it
class Label : public GuiElement
{
	public:
		Label(int w, int h) : nw(w), nh(h) { width = 0; height = 0; }
		void getNaturalSize(int& w, int& h) override { w = nw; h = nh; }
		void draw() override {}
		int nw, nh;
};

int main()
{
	// a row: positions follow natural sizes and spacing
	{
		Box a(50, 20), b(30, 40);
		GuiRow row;
		row.setSpacing(8).add(&a).add(&b);
		row.layout();
		CHECK(row.getWidth() == 88 && row.getHeight() == 40, "row fits its children: %d x %d", row.getWidth(), row.getHeight());
		CHECK(a.getLeft() == 0 && a.getTop() == 0 && b.getLeft() == 58 && b.getTop() == 0, "child positions %d,%d / %d,%d", a.getLeft(), a.getTop(), b.getLeft(), b.getTop());
		// a second pass changes nothing
		row.layout();
		CHECK(b.getLeft() == 58 && row.getWidth() == 88, "layout is idempotent");
	}
	// padding and cross-axis alignment
	{
		Box a(40, 20), b(40, 60);
		GuiRow row;
		row.setPadding(10).setAlignItems(LAYOUT_ALIGN::CENTRE).add(&a).add(&b);
		row.layout();
		CHECK(row.getWidth() == 100 && row.getHeight() == 80, "padded row: %d x %d", row.getWidth(), row.getHeight());
		CHECK(a.getTop() == 10 + 20 && b.getTop() == 10, "centered across: %d / %d", a.getTop(), b.getTop());
	}
	// a title bar: label, flexible space, label; and a grown, resized child
	{
		Box left(100, 24), right(150, 24);
		GuiRow bar(600, 0);
		bar.add(&left).addSpace().add(&right);
		bar.layout();
		CHECK(left.getLeft() == 0 && right.getLeft() == 450, "label at each end: %d / %d", left.getLeft(), right.getLeft());

		Box fixed(40, 10), stretchy(20, 10);
		GuiRow r2(300, 0);
		r2.add(&fixed).add(&stretchy, GuiItem().grow(1));
		r2.layout();
		CHECK(stretchy.getWidth() == 260 && stretchy.getLeft() == 40, "the growing child was resized: w=%d at %d", stretchy.getWidth(), stretchy.getLeft());
		// make the fixed one wider: the growing one gives way, without drifting
		fixed.setSize(100, 10);
		r2.layout();
		CHECK(stretchy.getWidth() == 200 && stretchy.getLeft() == 100, "reflows when a sibling grows: w=%d at %d", stretchy.getWidth(), stretchy.getLeft());
		fixed.setSize(40, 10);
		r2.layout();
		CHECK(stretchy.getWidth() == 260, "and returns when it shrinks again: w=%d", stretchy.getWidth());
	}
	// a label's size changing reflows the row by itself
	{
		Label t(60, 20); Box b(30, 20);
		GuiRow row;
		row.setSpacing(5).add(&t).add(&b);
		row.layout();
		CHECK(b.getLeft() == 65, "after the label: %d", b.getLeft());
		t.nw = 100;
		row.layout();
		CHECK(b.getLeft() == 105 && row.getWidth() == 135, "a longer label pushes the next child: %d, row %d", b.getLeft(), row.getWidth());
	}
	// hidden children take no room
	{
		Box a(30, 10), b(30, 10), c(30, 10);
		GuiColumn col;
		col.setSpacing(4).add(&a).add(&b).add(&c);
		col.layout();
		CHECK(col.getHeight() == 38 && c.getTop() == 28, "column of three: h=%d, third at %d", col.getHeight(), c.getTop());
		b.setVisible(false);
		col.layout();
		CHECK(col.getHeight() == 24 && c.getTop() == 14, "hiding the middle child closes the gap: h=%d, third at %d", col.getHeight(), c.getTop());
		b.setVisible(true);
		col.layout();
		CHECK(col.getHeight() == 38, "showing it again restores it");
	}
	// nesting settles in one pass: a card = overlay(background, column(row, row))
	{
		Box bg(1, 1), name(120, 20), batt(40, 14), chip(60, 18);
		GuiRow top; top.setSpacing(6).setAlignItems(LAYOUT_ALIGN::CENTRE).add(&name).add(&batt);
		GuiColumn body; body.setSpacing(8).add(&top).add(&chip);
		GuiOverlay card;
		card.setPadding(12).add(&bg, GuiItem().background()).add(&body);
		card.layout(); // one call on the outermost container
		CHECK(top.getWidth() == 166 && top.getHeight() == 20, "inner row: %d x %d", top.getWidth(), top.getHeight());
		CHECK(body.getWidth() == 166 && body.getHeight() == 46, "column: %d x %d", body.getWidth(), body.getHeight());
		CHECK(card.getWidth() == 190 && card.getHeight() == 70, "card: %d x %d", card.getWidth(), card.getHeight());
		CHECK(bg.getWidth() == 190 && bg.getHeight() == 70 && bg.getLeft() == 0, "backdrop fills the card: %d x %d", bg.getWidth(), bg.getHeight());
		CHECK(body.getLeft() == 12 && body.getTop() == 12, "content sits inside the padding: %d,%d", body.getLeft(), body.getTop());
		CHECK(batt.getLeft() == 12 + 126 && batt.getTop() == 12 + 3, "grandchild position accumulates through the parents: %d,%d", batt.getLeft(), batt.getTop());
	}
	// a grid of cards
	{
		Box a(80, 30), b(60, 30), c(80, 20), d(60, 40);
		GuiGrid grid(2);
		grid.setSpacing(10, 6).add(&a).add(&b).add(&c).add(&d);
		grid.layout();
		CHECK(grid.getWidth() == 150 && grid.getHeight() == 30 + 6 + 40, "grid size: %d x %d", grid.getWidth(), grid.getHeight());
		CHECK(b.getLeft() == 90 && c.getTop() == 36 && d.getLeft() == 90, "cells: b.x=%d c.y=%d d.x=%d", b.getLeft(), c.getTop(), d.getLeft());
	}
	// a column asked to stretch its children across its width
	{
		Box a(40, 10), b(100, 10);
		GuiColumn col;
		col.add(&a, GuiItem().stretch()).add(&b);
		col.layout();
		CHECK(a.getWidth() == 100 && b.getWidth() == 100, "stretched to the widest: %d / %d", a.getWidth(), b.getWidth());
	}
	// elements appended with the plain window call are picked up too, and removal reflows
	{
		Box a(10, 10), b(10, 10);
		GuiRow row;
		row.add(&a);
		row.append(&b); // not through add()
		row.layout();
		CHECK(b.getLeft() == 10, "a plain append() still takes part: %d", b.getLeft());
		row.remove(&a);
		row.layout();
		CHECK(b.getLeft() == 0 && row.getWidth() == 10, "removing a child reflows: %d", b.getLeft());
		row.clear();
		row.layout();
		CHECK(row.getWidth() == 0 && row.getHeight() == 0, "an empty row is empty");
	}
	// a window moves the selection through nested layouts by on-screen position
	{
		Box a(100, 30, true), b(100, 30, true), c(100, 30, true), d(100, 30, true);
		GuiRow r1; r1.setSpacing(10).add(&a).add(&b);
		GuiRow r2; r2.setSpacing(10).add(&c).add(&d);
		GuiColumn col; col.setSpacing(10).add(&r1).add(&r2);
		GuiWindow page(640, 480);
		page.append(&col);
		col.layout();
		a.setState(STATE::SELECTED);
		page.moveSelectionHor(1);
		CHECK(b.getState() == STATE::SELECTED && a.getState() == STATE::DEFAULT, "right: a -> b");
		page.moveSelectionVert(1);
		CHECK(d.getState() == STATE::SELECTED && b.getState() == STATE::DEFAULT, "down keeps the column: b -> d");
		page.moveSelectionHor(-1);
		CHECK(c.getState() == STATE::SELECTED, "left: d -> c");
		page.moveSelectionHor(-1);
		CHECK(b.getState() == STATE::SELECTED, "left off the start of a row wraps to the previous row's last item");
		page.moveSelectionVert(-1);
		CHECK(b.getState() == STATE::SELECTED, "up at the top stays put");
		d.setState(STATE::DISABLED);
		b.setState(STATE::SELECTED);
		page.moveSelectionVert(1);
		CHECK(d.getState() == STATE::DISABLED && c.getState() == STATE::SELECTED, "a disabled button cannot take the selection; down from b goes to c instead");
		// buttons of different heights in one row (centred) still count as the same row
		Box tall(100, 60, true), shortBox(100, 20, true);
		GuiRow mixed; mixed.setAlignItems(LAYOUT_ALIGN::CENTRE).setSpacing(10).add(&tall).add(&shortBox);
		GuiWindow page2(640, 480);
		page2.append(&mixed);
		mixed.layout();
		tall.setState(STATE::SELECTED);
		page2.moveSelectionHor(1);
		CHECK(shortBox.getState() == STATE::SELECTED, "right across a row of mixed heights");
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
