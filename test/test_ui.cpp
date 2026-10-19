// Host-side tests for the builder layer: UiScope ownership and bindings, GuiLabel, GuiChip, GuiTextButton, GuiCard.
// Built with AddressSanitizer, so a use-after-free in the teardown order fails the test. Run with `make -C test elements`.
#include "../source/libgui/Gui.h"
#include "../source/layoutdemo.h"
#include "../source/controllerdemo.h"
#include <cstdio>

extern bool g_triggerClicked;

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

// A page window is a child of the application's main window. Only the top-level window hands
// focus to child windows by itself, so the tests keep the page one level down, as the demo does.
struct Page
{
	GuiWindow root;
	GuiWindow page;
	Page() : root(640, 480), page(640, 480) { root.append(&page); }
};

// every element in a tree, parents before children
static void collect(GuiElement * e, std::vector<GuiElement *>& out)
{
	out.push_back(e);
	GuiWindow * w = dynamic_cast<GuiWindow *>(e);
	if(!w)
		return;
	for(uint32_t i = 0; i < w->getSize(); i++)
		collect(w->getGuiElementAt(i), out);
}

// exposes the sounds a GuiButton holds
class CueButton : public GuiButton
{
	public:
		CueButton() : GuiButton(10, 10) {}
		GuiSound * over() { return soundOver; }
		GuiSound * hold() { return soundHold; }
		GuiSound * click() { return soundClick; }
};

static int live = 0;
class Tracked : public GuiElement
{
	public:
		Tracked(int w = 10, int h = 10) { width = w; height = h; live++; }
		~Tracked() { live--; }
		void draw() override {}
};

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0); // so output survives a sanitizer abort
	// ---- ownership: everything the scope made goes with it, in any parent/child order
	{
		{
			UiScope ui;
			GuiColumn& parentFirst = ui.column();
			Tracked& a = ui.make<Tracked>();
			Tracked& b = ui.make<Tracked>();
			parentFirst.add(a).add(b);          // parent made before its children
			Tracked& c = ui.make<Tracked>();
			GuiRow& parentLast = ui.row();      // parent made after its child
			parentLast.add(c);
			CHECK(live == 3, "three tracked elements alive: %d", live);
		}
		CHECK(live == 0, "all destroyed with the scope: %d", live);
		{
			UiScope ui;
			Page host; GuiWindow& page = host.page;
			GuiColumn& col = ui.column();
			col.add(ui.make<Tracked>());
			page.append(&col);
			ui.attachTo(page);
			// the window is destroyed first here (declared after the scope), then the scope's elements
		}
		CHECK(live == 0, "a window that dies before the scope's elements is safe: %d", live);
		struct Thing { int* counter; Thing(int* c) : counter(c) { (*c)++; } ~Thing() { (*counter)--; } };
		int things = 0;
		{
			UiScope ui;
			ui.own<Thing>(&things);
			ui.own<Thing>(&things);
			CHECK(things == 2, "own() keeps non-element objects");
		}
		CHECK(things == 0, "own() objects are destroyed with the scope");
	}

	// ---- formatting
	CHECK(GuiFormat("battery %d%%", 80) == "battery 80%", "GuiFormat: %s", GuiFormat("battery %d%%", 80).c_str());
	CHECK(GuiFormat("%s", "") == "" && GuiFormat("x") == "x", "GuiFormat edge cases");

	// ---- bound labels follow their function, and reflow the layout
	{
		UiScope ui;
		Page host; GuiWindow& page = host.page;
		int pct = 80;
		GuiLabel& l = ui.label().bind([&]{ return GuiFormat("battery %d%%", pct); });
		Tracked& next = ui.make<Tracked>(10, 10);
		GuiRow& row = ui.row();
		row.gap(5).add(l).add(next);
		page.append(&row);
		ui.attachTo(page);
		page.update(nullptr);
		row.layout();
		CHECK(l.getLength() == 11, "initial text 'battery 80%%' has 11 characters: %d", l.getLength());
		const int before = next.getLeft();
		pct = 5;
		page.update(nullptr);
		row.layout();
		CHECK(l.getLength() == 10, "text follows the function: %d characters", l.getLength());
		CHECK(next.getLeft() == before - 10, "the shorter label pulls its neighbour in by one character (10): %d -> %d", before, next.getLeft());
		// an empty string clears the text rather than asking the translator for ""
		pct = 80;
		GuiLabel& e = ui.label().bind([&]{ return std::string(); });
		CHECK(e.getLength() == 0, "empty bound text");
	}

	// ---- visibleWhen / enabledWhen
	{
		UiScope ui;
		Page host; GuiWindow& page = host.page;
		bool show = true, enable = true;
		Tracked& a = ui.make<Tracked>(30, 10);
		Tracked& b = ui.make<Tracked>(30, 10);
		ui.visibleWhen(b, [&]{ return show; });
		GuiColumn& col = ui.column();
		col.gap(4).add(a).add(b);
		page.append(&col);
		ui.attachTo(page);
		page.update(nullptr); col.layout();
		CHECK(col.getHeight() == 24, "both visible: %d", col.getHeight());
		show = false;
		page.update(nullptr); col.layout();
		CHECK(!b.isVisible() && col.getHeight() == 10, "hidden by the binding, and the column closes up: %d", col.getHeight());
		show = true;
		page.update(nullptr); col.layout();
		CHECK(b.isVisible() && col.getHeight() == 24, "shown again");

		GuiTextButton& btn = ui.button("Go").enabledWhen([&]{ return enable; });
		page.append(&btn);
		page.update(nullptr);
		CHECK(btn.getState() == STATE::DEFAULT, "enabled");
		enable = false;
		page.update(nullptr);
		CHECK(btn.getState() == STATE::DISABLED, "disabled by the binding");
		enable = true;
		page.update(nullptr);
		CHECK(btn.getState() == STATE::DEFAULT, "enabled again");
	}

	// ---- text button: sizing, click and focus callbacks
	{
		UiScope ui;
		int clicks = 0, focuses = 0;
		GuiTextButton& btn = ui.button("Test").size(160, 40).onClick([&]{ clicks++; }).onFocus([&]{ focuses++; });
		int w, h;
		btn.getNaturalSize(w, h);
		CHECK(w == 160 && h == 40, "size(): %d x %d", w, h);

		btn.setState(STATE::SELECTED);
		btn.update(nullptr);
		CHECK(focuses == 1, "focus reported when selected: %d", focuses);
		btn.update(nullptr);
		CHECK(focuses == 1, "and not again while it stays selected: %d", focuses);
		btn.resetState();
		btn.update(nullptr);
		btn.setState(STATE::SELECTED);
		btn.update(nullptr);
		CHECK(focuses == 2, "selecting it again reports again: %d", focuses);

		btn.setState(STATE::CLICKED);
		btn.update(nullptr);
		CHECK(clicks == 1 && btn.getState() == STATE::SELECTED, "a click runs the callback and leaves the button selected: clicks=%d state=%d", clicks, (int)btn.getState());
		btn.update(nullptr);
		CHECK(clicks == 1, "once");
		CHECK(focuses == 2, "a click does not count as a new focus: %d", focuses);

		// without a callback, the state is left for the caller to poll
		GuiTextButton& plain = ui.button("Plain");
		CHECK(&plain.litTextColor(PixelColor{10, 20, 30, 255}) == &plain, "litTextColor continues the chain");
		plain.setState(STATE::CLICKED);
		plain.update(nullptr);
		CHECK(plain.getState() == STATE::CLICKED, "no onClick: CLICKED stays for polling");

		// layout resizes a button and its text stays centred (the button owns the shape)
		GuiRow& row = ui.row();
		row.size(400, 0).add(btn, GuiItem().grow(1)).add(plain);
		row.layout();
		btn.getNaturalSize(w, h);
		CHECK(w > 160, "a growing button is resized by layout: %d", w);

		btn.fontSize(14);
		btn.setText("Longer");
		btn.disabled();
		CHECK(btn.getState() == STATE::DISABLED, "disabled()");
		btn.disabled(false);
		CHECK(btn.getState() == STATE::DEFAULT, "disabled(false)");
	}

	// ---- cards: size, selection through the D-pad, click and focus callbacks
	{
		UiScope ui;
		Page host; GuiWindow& page = host.page;
		int focused = -1, clicked = -1;
		GuiCard& c0 = ui.card().size(300, 0).onFocus([&]{ focused = 0; }).onClick([&]{ clicked = 0; }).add(ui.label("Player one"));
		GuiCard& c1 = ui.card().size(300, 0).onFocus([&]{ focused = 1; }).onClick([&]{ clicked = 1; }).add(ui.label("Player two"));
		GuiCard& panel = ui.card().selectable(false).add(ui.label("Not selectable"));
		GuiColumn& col = ui.column();
		col.gap(10).add(c0).add(c1).add(panel);
		page.append(&col);
		ui.attachTo(page);
		page.update(nullptr);
		col.layout();

		const int pad = (int)GuiTheme::current().padding;
		CHECK(c0.getWidth() == 300, "card width: %d", c0.getWidth());
		CHECK(c0.getHeight() == 24 + 2 * pad, "card height fits its content (24 text + padding): %d", c0.getHeight());
		GuiElement * s0 = c0.getGuiElementAt(0);
		CHECK(s0 && s0->getWidth() == 300 && s0->getHeight() == c0.getHeight(), "the surface fills the card: %d x %d", s0 ? s0->getWidth() : -1, s0 ? s0->getHeight() : -1);
		CHECK(s0 && s0->getLeft() == c0.getLeft() && s0->getTop() == c0.getTop(), "and sits at its corner");
		CHECK(c0.getState() == STATE::DEFAULT, "the card itself is not the selected element");

		page.moveSelectionVert(1);
		page.update(nullptr);
		CHECK(s0->getState() == STATE::SELECTED && focused == 0, "the D-pad selects the first card: focused=%d", focused);
		page.moveSelectionVert(1);
		page.update(nullptr);
		GuiElement * s1 = c1.getGuiElementAt(0);
		CHECK(s1->getState() == STATE::SELECTED && s0->getState() == STATE::DEFAULT && focused == 1, "down moves to the second card: focused=%d", focused);
		page.moveSelectionVert(1);
		page.update(nullptr);
		CHECK(s1->getState() == STATE::SELECTED, "the plain panel is skipped");
		page.moveSelectionVert(-1);
		page.update(nullptr);
		CHECK(s0->getState() == STATE::SELECTED && focused == 0, "up goes back");
		c0.deselect();
		CHECK(!c0.isSelected() && s0->getState() == STATE::DEFAULT, "deselect takes the selection away");
		c1.select();
		CHECK(c1.isSelected() && !c0.isSelected(), "select gives the card the selection");
		s1->setState(STATE::HELD);
		CHECK(c1.isSelected(), "a held card still counts as selected");
		c1.deselect();

		s0->setState(STATE::CLICKED);
		page.update(nullptr);
		CHECK(clicked == 0 && s0->getState() == STATE::SELECTED, "clicking a card runs its callback: clicked=%d", clicked);

		// disabling a card disables its surface, not the window (which would dim the whole screen)
		bool on = false;
		c1.enabledWhen([&]{ return on; });
		page.update(nullptr);
		CHECK(c1.getState() == STATE::DEFAULT && s1->getState() == STATE::DISABLED, "disabled card: window state %d, surface state %d", (int)c1.getState(), (int)s1->getState());
		page.moveSelectionVert(1);
		CHECK(s1->getState() == STATE::DISABLED && s0->getState() == STATE::SELECTED, "a disabled card cannot be selected");
		on = true;
		page.update(nullptr);
		CHECK(s1->getState() == STATE::DEFAULT, "enabled again");
	}

	// ---- selection skips hidden elements
	{
		UiScope ui;
		Page host; GuiWindow& page = host.page;
		bool showMiddle = false;
		GuiTextButton& a = ui.button("A").size(100, 30);
		GuiTextButton& b = ui.button("B").size(100, 30).visibleWhen([&]{ return showMiddle; });
		GuiTextButton& c = ui.button("C").size(100, 30);
		GuiColumn& col = ui.column();
		col.gap(8).add(a).add(b).add(c);
		page.append(&col);
		ui.attachTo(page);
		page.update(nullptr);
		col.layout();
		a.setState(STATE::SELECTED);
		page.moveSelectionVert(1);
		CHECK(c.getState() == STATE::SELECTED && b.getState() == STATE::DEFAULT, "a hidden button is skipped: C selected, B not");
		showMiddle = true;
		page.update(nullptr);
		col.layout();
		page.moveSelectionVert(-1);
		CHECK(b.getState() == STATE::SELECTED, "and reachable once shown");
	}

	// ---- chips
	{
		UiScope ui;
		Page host; GuiWindow& page = host.page;
		std::string role = "Joystick 1";
		GuiChip& chip = ui.chip().bind([&]{ return role; });
		page.append(&chip);
		ui.attachTo(page);
		page.update(nullptr);
		chip.layout();
		const int w1 = chip.getWidth();
		CHECK(w1 == 10 * 8 / 1 + 0 || w1 > 0, "chip has a size: %d", w1);
		role = "Keys + mouse";
		page.update(nullptr);
		chip.layout();
		CHECK(chip.getWidth() > w1, "the chip grows with its text: %d -> %d", w1, chip.getWidth());
	}

	// ---- the whole point: a screen reads as a description
	{
		UiScope ui;
		Page host; GuiWindow& page = host.page;
		int pct = 80;
		GuiCard& card = ui.card().size(500, 0)
			.add(ui.column().gap(6)
				.add(ui.row().size(476, 0).gap(8).align(LAYOUT_ALIGN::CENTRE)
					.add(ui.label("P1").fontSize(22).accent())
					.add(ui.label("Wiimote").bind([]{ return std::string("Wiimote"); }), GuiItem().grow(1))
					.add(ui.label().bind([&]{ return GuiFormat("battery %d%%", pct); }).fontSize(16).muted())));
		page.append(&card);
		ui.attachTo(page);
		page.update(nullptr);
		card.layout();
		CHECK(card.getWidth() == 500 && card.getHeight() > 0, "a nested card: %d x %d", card.getWidth(), card.getHeight());
	}

	// ---- the Layout demo screen, built for real: it fits, and the D-pad reaches everything it should
	{
		Page host;
		GuiLayoutDemo demo(nullptr);
		host.page.append(&demo);
		host.page.update(nullptr);
		demo.layout();
		CHECK(demo.getWidth() == 592 && demo.getTop() + demo.getHeight() <= 480, "the screen fits: %d x %d at y=%d", demo.getWidth(), demo.getHeight(), demo.getTop());

		// every button in the tree, in the order the demo builds them
		std::vector<GuiElement *> all, btns;
		collect(&demo, all);
		for(GuiElement * e : all)
			if(dynamic_cast<GuiButton *>(e))
				btns.push_back(e);
		CHECK(btns.size() == 9, "two cards, the panel's surface and six buttons: %zu", btns.size());
		GuiElement * card0 = btns[0], * card1 = btns[1], * panel = btns[2], * test = btns[3], * reset = btns[5], * identify = btns[6], * config = btns[7], * back = btns[8];

		CHECK(!panel->isSelectable(), "the panel cannot be selected");
		CHECK(card0->isSelectable() && card1->isSelectable() && test->isSelectable(), "cards and buttons can");
		CHECK(identify->getState() == STATE::DISABLED && !identify->isSelectable(), "Identify starts disabled, so it is skipped");

		host.page.moveSelectionVert(1);
		host.page.update(nullptr);
		CHECK(card0->getState() == STATE::SELECTED, "first selection is the first card");
		CHECK(identify->getState() == STATE::DEFAULT, "visiting a card enabled Identify (the binding ran in the same frame)");
		host.page.moveSelectionVert(1);
		host.page.update(nullptr);
		CHECK(card1->getState() == STATE::SELECTED && card0->getState() == STATE::DEFAULT, "down: second card");
		host.page.moveSelectionVert(1);
		host.page.update(nullptr);
		GuiElement * map = btns[4];
		CHECK(map->getState() == STATE::SELECTED && panel->getState() == STATE::DISABLED, "down skips the panel and lands on the button nearest the card's centre (Map)");
		host.page.moveSelectionHor(1);
		CHECK(reset->getState() == STATE::SELECTED, "right: Reset");
		host.page.moveSelectionHor(-1);
		host.page.moveSelectionHor(-1);
		CHECK(test->getState() == STATE::SELECTED, "left, left: Test");
		host.page.moveSelectionVert(1);
		host.page.update(nullptr);
		CHECK(identify->getState() == STATE::SELECTED, "down from Test reaches Identify");
		host.page.moveSelectionHor(1);
		CHECK(config->getState() == STATE::SELECTED, "right: Config");
		host.page.moveSelectionHor(1);
		CHECK(back->getState() == STATE::SELECTED, "right: Go Back");
		host.page.moveSelectionVert(-1);
		CHECK(reset->getState() == STATE::SELECTED, "up from Go Back: Reset, the button above it");

		demo.backButton().setState(STATE::CLICKED);
		host.page.update(nullptr);
		CHECK(demo.backButton().getState() == STATE::CLICKED, "Go Back has no callback, so the page loop can poll it as before");
	}

	// ---- the controller configuration screen
	{
		Page host;
		GuiControllerDemo demo(nullptr);
		host.page.append(&demo);
		host.page.update(nullptr);
		demo.layout();
		CHECK(demo.getWidth() == 624 && demo.getTop() + demo.getHeight() <= 480, "the screen fits: %d x %d at y=%d", demo.getWidth(), demo.getHeight(), demo.getTop());

		std::vector<GuiElement *> all, btns;
		collect(&demo, all);
		for(GuiElement * e : all)
			if(dynamic_cast<GuiButton *>(e))
				btns.push_back(e);
		// 4 + 4 side rows, 2 + 4 + 2 + 4 group rows, the bar's surface, and Back
		CHECK(btns.size() == 4 + 4 + 12 + 1 + 1, "buttons and rows: %zu", btns.size());
		CHECK(demo.highlighted() == 0, "nothing highlighted at first");

		host.page.moveSelectionVert(1);
		host.page.update(nullptr);
		CHECK(demo.highlighted() == INPUT_TRIGGER_L, "the first row is the left bumper and its control is highlighted: %d", demo.highlighted());
		host.page.moveSelectionVert(1);
		host.page.update(nullptr);
		CHECK(demo.highlighted() == INPUT_TRIGGER_ZL, "down: left trigger");
		host.page.moveSelectionHor(1);
		host.page.update(nullptr);
		CHECK(demo.highlighted() == INPUT_TRIGGER_R || demo.highlighted() == INPUT_TRIGGER_ZR, "right crosses the controller to the right-hand list: %d", demo.highlighted());

		// every highlightable row maps to a control the layout really has
		const ControllerLayout& pro = ProControllerLayout();
		for(int i = 0; i < 40; i++)
		{
			host.page.moveSelectionVert(1);
			host.page.update(nullptr);
			if(demo.highlighted() != 0)
				CHECK(pro.find(demo.highlighted()) != nullptr, "highlighted control %d exists on the layout", demo.highlighted());
		}
		// walk the whole screen from the last row back to the first, in every direction
		for(int i = 0; i < 12; i++) host.page.moveSelectionHor(1);
		for(int i = 0; i < 12; i++) { host.page.moveSelectionHor(-1); host.page.update(nullptr); }
		CHECK(demo.highlighted() != 0, "selection is somewhere sensible after wandering");
	}

	// default sound cues: registered once, picked up by every button and builder surface made afterwards
	{
		CHECK(GuiSound::getDefaultOver() == nullptr && GuiSound::getDefaultClick() == nullptr, "no sounds registered: silent by default");
		GuiSound over, click, other;
		GuiSound::setDefaultSounds(&over, &click);
		CHECK(GuiSound::getDefaultOver() == &over && GuiSound::getDefaultClick() == &click, "defaults registered");
		{
			CueButton plain;
			CHECK(plain.over() == &over && plain.click() == &click && plain.hold() == nullptr, "a new button plays the defaults");
			CueButton replaced;
			replaced.setSoundOver(&other);
			CHECK(replaced.over() == &other && replaced.click() == &click, "a button can override one sound");
			CueButton silenced;
			silenced.setSoundOver(nullptr);
			CHECK(silenced.over() == nullptr && silenced.click() == &click, "nullptr silences one sound");

			UiScope scope;
			CHECK(scope.getSoundOver() == &over && scope.getSoundClick() == &click, "a UiScope starts from the defaults");
			scope.setSounds(nullptr, nullptr);
			CHECK(scope.getSoundOver() == nullptr && scope.getSoundClick() == nullptr, "a UiScope can be made silent");
		}
		CueButton before;
		GuiSound::setDefaultSounds(nullptr, nullptr);
		CueButton after;
		CHECK(before.over() == &over && after.over() == nullptr && after.click() == nullptr, "changing the defaults affects only buttons made afterwards");
	}

	// plain value types start zeroed, so a default-constructed one is never garbage
	{
		GuiPoint * p = new GuiPoint;
		GuiRect * r = new GuiRect;
		GuiFill * f = new GuiFill;
		GuiMeshKey * k = new GuiMeshKey;
		OptionList * o = new OptionList;
		CHECK(p->x == 0 && p->y == 0, "GuiPoint defaults to the origin");
		CHECK(r->x == 0 && r->y == 0 && r->w == 0 && r->h == 0, "GuiRect defaults to empty");
		const GuiRect lit = { 1, 2, 3, 4 };
		CHECK(lit.x == 1 && lit.h == 4, "GuiRect still takes {x, y, w, h}");
		CHECK(!f->gradient && f->angle == 0 && f->from.a == 0 && f->to.a == 0, "GuiFill defaults to a transparent solid");
		CHECK(k->width == 0 && k->height == 0 && k->feather == 0 && k->tolerance == 0, "GuiMeshKey defaults to empty");
		CHECK(o->length == 0 && o->name[0][0] == 0 && o->value[MAX_OPTIONS - 1][49] == 0, "OptionList defaults to no entries");
		delete p; delete r; delete f; delete k; delete o;
	}

	CHECK(live == 0, "nothing leaked: %d", live);
	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
