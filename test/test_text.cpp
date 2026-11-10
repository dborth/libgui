// Host test of GuiText: how a text is stored, cut off, wrapped and drawn. The real GuiText runs against the stub
// renderer in host_stubs.cpp, where every character is half its pixel size wide (10 px at size 20) and every
// drawText() call is recorded, so what is on each line can be checked exactly. Scrolling is not covered here: it
// needs a frame timer, which the stubs do not have (the library is exercised with a real font and timer elsewhere).
#include "libgui/Gui.h"
#include <cstdio>
#include <string>
#include <vector>

extern std::vector<std::wstring> g_drawnText;

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

typedef std::vector<std::wstring> Lines;
static const PixelColor WHITE = { 255, 255, 255, 255 };

static Lines drawn(GuiText & t)
{
	g_drawnText.clear();
	t.draw();
	return g_drawnText;
}

static Lines L(std::initializer_list<const wchar_t *> l)
{
	Lines v;
	for(const wchar_t * s : l) v.push_back(s);
	return v;
}

static std::string show(const Lines & l)
{
	std::string s;
	for(const std::wstring & w : l) { s += "["; for(wchar_t c : w) s += (char)c; s += "]"; }
	return s;
}

#define CHECK_LINES(t, ...) do { const Lines got = drawn(t); const Lines want = L(__VA_ARGS__); CHECK(got == want, "got %s, want %s", show(got).c_str(), show(want).c_str()); } while(0)

int main()
{
	// no width: the whole text, as given
	{ GuiText t("Hello", 20, WHITE); CHECK_LINES(t, { L"Hello" }); CHECK(t.getLength() == 5 && t.getTextWidth() == 50, "5 chars, 50px: %d %d", t.getLength(), t.getTextWidth()); }

	// a maximum width without wrapping keeps what fits
	{ GuiText t("abcdefghij", 20, WHITE); t.setMaxWidth(55); CHECK_LINES(t, { L"abcde" }); t.setMaxWidth(100); CHECK_LINES(t, { L"abcdefghij" }); t.setMaxWidth(0); CHECK_LINES(t, { L"abcdefghij" }); }

	// wrapping breaks at spaces; a line is measured without the spaces that end it
	{ GuiText t("alpha beta gamma delta", 20, WHITE); t.setWrap(true, 160); CHECK_LINES(t, { L"alpha beta gamma", L"delta" }); }
	{ GuiText t("alpha beta gamma delta", 20, WHITE); t.setWrap(true, 159); CHECK_LINES(t, { L"alpha beta", L"gamma delta" }); }
	{ GuiText t("one two three four five", 20, WHITE); t.setWrap(true, 80); CHECK_LINES(t, { L"one two", L"three", L"four", L"five" }); }
	// a word wider than the line gets one to itself, and the text goes on after it
	{ GuiText t("supercalifragilistic ok", 20, WHITE); t.setWrap(true, 50); CHECK_LINES(t, { L"supercalifragilistic", L"ok" }); }
	// spaces at the end of a line, or of the text, are not drawn; a first line keeps the spaces it starts with
	{ GuiText t("hi   there", 20, WHITE); t.setWrap(true, 40); CHECK_LINES(t, { L"hi", L"there" }); }
	{ GuiText t("hi   ", 20, WHITE); t.setWrap(true, 1000); CHECK_LINES(t, { L"hi" }); }
	{ GuiText t("  hi there", 20, WHITE); t.setWrap(true, 1000); CHECK_LINES(t, { L"  hi there" }); }
	// a line break character is just a character
	{ GuiText t("a\nb c", 20, WHITE); t.setWrap(true, 1000); CHECK_LINES(t, { L"a\nb c" }); }

	// as much text as there are lines for is drawn, and what is left over is replaced by "..." on the last line
	{
		std::string s; for(int i = 0; i < 16; i++) s += i ? " word" : "word";
		GuiText t(s.c_str(), 20, WHITE); t.setWrap(true, 100);
		CHECK_LINES(t, { L"word word", L"word word", L"word word", L"word word", L"word word", L"word word", L"word word", L"word word" });
		s += " more"; t.setText(s.c_str());
		CHECK_LINES(t, { L"word word", L"word word", L"word word", L"word word", L"word word", L"word word", L"word word", L"word wo..." });
		t.setWrap(true, 20); // narrower than the "..." itself
		const Lines got = drawn(t);
		CHECK(got.size() == 8 && got.back() == L"...", "nothing fits beside the dots: %s", show(got).c_str());
	}

	// changing anything that decides where lines break makes the text break again
	{
		GuiText t("aaaa bbbb", 20, WHITE); t.setWrap(true, 100);
		CHECK_LINES(t, { L"aaaa bbbb" });
		t.setFontSize(40); CHECK_LINES(t, { L"aaaa", L"bbbb" });
		t.setFontSize(20); t.setWrap(true, 60); CHECK_LINES(t, { L"aaaa", L"bbbb" });
		t.setWrap(true, 60); CHECK_LINES(t, { L"aaaa", L"bbbb" });
		t.setWrap(false, 60); CHECK_LINES(t, { L"aaaa b" });
		t.setText("cc dd"); t.setWrap(true, 40); CHECK_LINES(t, { L"cc", L"dd" });
	}

	// replacing the text: longer, shorter, empty, none, longer again
	{
		GuiText t("start", 20, WHITE);
		t.setText("a considerably longer text than before"); CHECK(t.getLength() == 38, "%d", t.getLength()); CHECK_LINES(t, { L"a considerably longer text than before" });
		t.setText("short"); CHECK(t.getLength() == 5, "%d", t.getLength()); CHECK_LINES(t, { L"short" });
		t.setText(""); CHECK(t.getLength() == 0, "%d", t.getLength());
		t.setText(nullptr); CHECK(t.getLength() == 0 && drawn(t).empty() && t.getTextWidth() == 0, "no text draws nothing");
		t.resetText(); CHECK(t.getLength() == 0, "nothing to translate again");
		t.setText("back"); t.resetText(); CHECK_LINES(t, { L"back" });
	}

	// words
	{ GuiText t("ab cde  f", 20, WHITE); CHECK(t.getLongestWordWidth() == 30, "%d", t.getLongestWordWidth()); GuiText e("   ", 20, WHITE); CHECK(e.getLongestWordWidth() == 0, "no words"); GuiText n(nullptr, 20, WHITE); CHECK(n.getLongestWordWidth() == 0, "no text"); }

	// text without breaks of any kind, and unusual input, must neither loop nor draw outside the lines kept
	{
		const char * odd[] = { "", " ", "     ", "x", "  lead", "trail   ", "a  b   c" };
		const int widths[] = { -5, 0, 1, 10, 40, 1000 };
		for(const char * s : odd) for(int w : widths)
		{
			GuiText t(s, 20, WHITE); t.setWrap(true, w); const Lines got = drawn(t);
			CHECK(got.size() <= 8, "\"%s\" at %d: %zu lines", s, w, got.size());
			for(const std::wstring & l : got) CHECK(l.size() <= strlen(s) + 3, "\"%s\" at %d: line of %zu", s, w, l.size());
		}
	}

	// long text is cut to a fixed length rather than trusted
	{
		std::string words; for(int i = 0; i < 1000; i++) words += "xxxx ";
		GuiText t(words.c_str(), 20, WHITE); CHECK(t.getLength() == 1023, "%d", t.getLength());
		t.setWrap(true, 100); const Lines got = drawn(t);
		CHECK(got.size() == 8 && got.back() == L"xxxx xx...", "%s", show(got).c_str());
		std::string one(5000, 'x'); GuiText w(one.c_str(), 20, WHITE); w.setWrap(true, 100);
		const Lines single = drawn(w); CHECK(single.size() == 1 && single[0].size() == 1023, "one word, one line: %zu lines", single.size());
	}

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
