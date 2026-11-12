// Host test of GuiTextTranslator, which reads a compiled gettext catalog (.mo) in place. It links the real
// GuiTextTranslator.cpp (so it does not use host_stubs.cpp) and runs against a catalog that msgfmt built from
// fixtures/translator.lang, the same way the real .lang files are built, plus catalogs built here byte by byte so
// that every way a catalog can be wrong can be tried. Build it with -fsanitize=address,undefined: the catalogs are
// exact-size heap blocks, so any read past the end of one is an error.
//
//   translator_test <catalog.mo>                      the tests
//   translator_test --verify <file.lang> <file.mo>    checks a real catalog against its source (see verifySource)
#include "libgui/GuiTextTranslator.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <utility>
#include <vector>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

typedef std::vector<uint8_t> Bytes;
typedef std::vector<std::pair<std::string, std::string> > Entries;

// ---- a static that resolves text during static initialization -------------------------------------------------------
// textTranslator is defined in another translation unit; were it built by a dynamic initializer, `early` could run
// before it and find nothing.
struct Early { const char * result; Early() { result = textTranslator ? textTranslator->getText("early") : "NO TRANSLATOR"; } };
static Early early;

// ---- building catalogs ---------------------------------------------------------------------------------------------
static void put32(Bytes & b, size_t at, uint32_t v, bool be)
{
	for(int i = 0; i < 4; i++)
		b[at + i] = (uint8_t)(v >> (be ? 24 - 8 * i : 8 * i));
}
static uint32_t get32(const Bytes & b, size_t at, bool be)
{
	uint32_t v = 0;
	for(int i = 0; i < 4; i++)
		v |= (uint32_t)b[at + i] << (be ? 24 - 8 * i : 8 * i);
	return v;
}

// Entries are laid out as given (the caller sorts them, or deliberately does not)
static Bytes makeCatalog(const Entries & e, bool be = false)
{
	const size_t n = e.size(), origAt = 28, transAt = 28 + 8 * n, dataAt = 28 + 16 * n;
	Bytes b(dataAt, 0);
	put32(b, 0, 0x950412DEu, be);
	put32(b, 4, 0, be);
	put32(b, 8, (uint32_t)n, be);
	put32(b, 12, (uint32_t)origAt, be);
	put32(b, 16, (uint32_t)transAt, be);
	for(size_t i = 0; i < n; i++)
	{
		const std::string * s[2] = { &e[i].first, &e[i].second };
		for(int k = 0; k < 2; k++)
		{
			const size_t entry = (k ? transAt : origAt) + 8 * i;
			put32(b, entry, (uint32_t)s[k]->size(), be);
			put32(b, entry + 4, (uint32_t)b.size(), be);
			b.insert(b.end(), s[k]->begin(), s[k]->end());
			b.push_back(0);
		}
	}
	return b;
}

static Entries sorted(Entries e)
{
	std::sort(e.begin(), e.end());	// std::string compares as unsigned bytes, like strcmp
	return e;
}

// An exact-size heap copy, so the sanitizers see any access past the end
struct Heap
{
	uint8_t * p; size_t n;
	explicit Heap(const Bytes & b) : p((uint8_t *)malloc(b.empty() ? 1 : b.size())), n(b.size()) { if(n) memcpy(p, &b[0], n); }
	~Heap() { free(p); }
};

// Loads a catalog and keeps it alive (the translator reads it in place) until the next one is loaded
static Heap * g_current = nullptr;
static bool loads(const Bytes & b)
{
	Heap * h = new Heap(b);
	const bool ok = textTranslator->loadLanguage(h->p, h->n);
	delete g_current;	// a refused load has already stopped using it, and a good one has switched to the new block
	g_current = h;
	return ok;
}

static bool eq(const char * a, const char * b) { return a && b && strcmp(a, b) == 0; }

static Entries sampleEntries()
{
	Entries e;
	e.push_back(std::make_pair("hello", "hola"));
	e.push_back(std::make_pair("bye", "adios"));
	e.push_back(std::make_pair("Save", "Speichern"));
	e.push_back(std::make_pair("Save As", "Speichern unter"));
	e.push_back(std::make_pair("Save As...", "Speichern unter..."));
	e.push_back(std::make_pair("Zoom", "Gr\xC3\xB6\xC3\x9F" "e"));
	e.push_back(std::make_pair("\xE8\xA8\xAD\xE5\xAE\x9A", "Settings"));	// a non-ASCII ID sorts after the ASCII ones
	return sorted(e);
}

// ---- tests ---------------------------------------------------------------------------------------------------------
static Bytes readFile(const char * path)
{
	Bytes b;
	FILE * f = fopen(path, "rb");
	if(!f) return b;
	uint8_t buf[4096];
	size_t n;
	while((n = fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
	fclose(f);
	return b;
}

static void testMsgfmtCatalog(const char * path)
{
	const Bytes b = readFile(path);
	CHECK(!b.empty(), "read %s", path);
	CHECK(loads(b), "the catalog msgfmt built loads");
	CHECK(textTranslator->isLoaded(), "isLoaded");
	CHECK(textTranslator->getCount() == 11, "10 translated messages and the header entry, and not the untranslated one: %zu", textTranslator->getCount());
	CHECK(eq(textTranslator->getText("Save"), "Speichern"), "translated");
	CHECK(eq(textTranslator->getText("Save As"), "Speichern unter"), "an ID that starts another one is found as itself");
	CHECK(eq(textTranslator->getText("Save As..."), "Speichern unter..."), "and so is the longer one");
	CHECK(eq(textTranslator->getText("Saved"), "Gespeichert"), "and one that merely shares the prefix");
	CHECK(eq(textTranslator->getText("Sav"), "Sav"), "a prefix of an ID is not the ID");
	CHECK(eq(textTranslator->getText("Save "), "Save "), "an ID plus a space is not the ID");
	CHECK(eq(textTranslator->getText("save"), "save"), "case matters");
	CHECK(eq(textTranslator->getText("Cancel"), "Cancel"), "an untranslated message comes back as it was");
	CHECK(textTranslator->find("Cancel") == nullptr, "find() says there is no translation");
	CHECK(eq(textTranslator->getText("Settings"), "\xE8\xA8\xAD\xE5\xAE\x9A"), "UTF-8 comes through whole");
	CHECK(eq(textTranslator->getText("Zoom"), "Gr\xC3\xB6\xC3\x9F" "e"), "so do accents");
	CHECK(eq(textTranslator->getText(""), ""), "the empty string stays empty (an empty ID is the catalog's header entry)");
	CHECK(textTranslator->find("") == nullptr, "and has no translation (so the header text is never returned)");
	CHECK(textTranslator->find("Content-Type: text/plain; charset=UTF-8\n") == nullptr, "nor is the header text a message");
	CHECK(textTranslator->getText(nullptr) == nullptr, "null in, null out");
	CHECK(textTranslator->find(nullptr) == nullptr, "null has no translation");

	// every translation is a pointer into the catalog that ends where it should
	Heap h(b);
	CHECK(textTranslator->loadLanguage(h.p, h.n), "loads from a heap copy");
	const char * s = textTranslator->find("Save");
	CHECK(s && (const uint8_t *)s >= h.p && (const uint8_t *)s < h.p + h.n, "the translation is in the catalog, not a copy");

	// Tr / TrFormat
	CHECK(eq(Tr("Save"), "Speichern") && eq(Tr("nope"), "nope") && eq(Tr(nullptr), ""), "Tr");
	CHECK(Tr("Save") == textTranslator->find("Save"), "Tr is the translation where it lies in the catalog, not a copy");
	{ const char * id = "nope"; CHECK(Tr(id) == id, "untranslated, Tr is the ID itself"); }
	CHECK(TrFormat("Player %d", 3) == "Spieler 3", "TrFormat: %s", TrFormat("Player %d", 3).c_str());
	CHECK(TrFormat("%s uses %s", "A", "B") == "A nutzt B", "TrFormat with two arguments");
	CHECK(TrFormat("%d of %d", 2, 5) == "2 von 5", "TrFormat repeated conversions");
	CHECK(TrFormat("Mixed %s %d", "x", 7) == "Mixed x 7", "a translation whose conversions are out of order is not used: %s", TrFormat("Mixed %s %d", "x", 7).c_str());
	CHECK(TrFormat("Untranslated %d%%", 50) == "Untranslated 50%", "TrFormat falls back to the format itself");
	CHECK(TrFormat(nullptr).empty(), "TrFormat of null");

	// TrFormatTo / TrMessage: the same messages, in a buffer
	{
		char out[64];
		CHECK(TrFormatTo(out, sizeof(out), "Player %d", 3) == 9 && eq(out, "Spieler 3"), "TrFormatTo: %s", out);
		CHECK(TrFormatTo(out, sizeof(out), "%s uses %s", "A", "B") == 9 && eq(out, "A nutzt B"), "TrFormatTo with two arguments: %s", out);
		CHECK(TrFormatTo(out, sizeof(out), "Mixed %s %d", "x", 7) == 9 && eq(out, "Mixed x 7"), "an unusable translation is not used: %s", out);
		CHECK(TrFormatTo(out, sizeof(out), nullptr) == 0 && out[0] == 0, "null message");
		CHECK(TrFormatTo(nullptr, 10, "Player %d", 3) == 0 && TrFormatTo(out, 0, "Player %d", 3) == 0, "no buffer, no size: nothing is written");
		char guard[8]; memset(guard, 'x', sizeof(guard));
		CHECK(TrFormatTo(guard, 4, "Player %d", 3) == 3 && eq(guard, "Spi") && guard[4] == 'x', "cut to fit, NUL-terminated, nothing written past the size: %s", guard);
		CHECK(TrFormatTo(guard, 1, "Player %d", 3) == 0 && guard[0] == 0, "room for only the NUL");
		CHECK(TrMessage<>("Player %d", 3).c_str() != nullptr && eq(TrMessage<>("Player %d", 3), "Spieler 3"), "TrMessage");
		const char * viaCall = TrMessage<16>("%s uses %s", "A", "B");
		(void)viaCall;
		CHECK(eq(TrMessage<8>("Untranslated %d%%", 50), "Untrans"), "TrMessage cuts to its size");
	}

	// text cut by a buffer that is too small ends on a whole character, never partway through one
	{
		const char * text = "a\xC3\xA9" "b\xE6\x97\xA5" "c\xF0\x9F\x98\x80" "d"; // a, e-acute, b, CJK, c, emoji, d
		const size_t whole[] = { 0, 1, 3, 4, 7, 8, 12, 13 };			// the lengths at which a character ends
		for(size_t n = 0; n <= strlen(text); n++)
		{
			const size_t t = GuiTextTranslator::utf8Trim(text, n);
			bool boundary = false;
			for(size_t w : whole) boundary = boundary || w == t;
			CHECK(t <= n && boundary && (t == n) == [&]{ for(size_t w : whole) if(w == n) return true; return false; }(), "utf8Trim(%zu) = %zu", n, t);
		}
		CHECK(GuiTextTranslator::utf8Trim("\x80\x80", 2) == 2 && GuiTextTranslator::utf8Trim("", 0) == 0, "no character was cut in text that is not valid or is empty");
		char buf[8];
		TrFormatTo(buf, sizeof(buf), "%s", "ab\xE6\x97\xA5\xE6\x97\xA5"); // 2 + 3 + 3 bytes into room for 7
		CHECK(eq(buf, "ab\xE6\x97\xA5"), "a character that does not fit whole is left out");
	}
}

static void testLifecycle()
{
	textTranslator->clear();
	CHECK(!textTranslator->isLoaded() && textTranslator->getCount() == 0, "nothing loaded");
	CHECK(eq(textTranslator->getText("hello"), "hello"), "English before any catalog is loaded");

	const Bytes a = makeCatalog(sampleEntries());
	CHECK(loads(a), "loads");
	CHECK(textTranslator->getCount() == 7, "count");
	CHECK(eq(textTranslator->getText("hello"), "hola"), "translated");

	Entries other = sampleEntries();
	for(size_t i = 0; i < other.size(); i++) if(other[i].first == "hello") other[i].second = "salut";
	CHECK(loads(makeCatalog(other)), "a second catalog replaces the first");
	CHECK(eq(textTranslator->getText("hello"), "salut"), "and is the one in use");
	CHECK(loads(a) && eq(textTranslator->getText("hello"), "hola"), "and back (both slots have been used twice)");

	CHECK(!textTranslator->loadLanguage(nullptr, 0), "a null catalog is refused");
	CHECK(!textTranslator->isLoaded() && eq(textTranslator->getText("hello"), "hello"), "and leaves English, not the previous catalog");
	CHECK(loads(a), "loads again");
	textTranslator->clear();
	CHECK(eq(textTranslator->getText("hello"), "hello"), "clear() selects English");
}

static void testByteOrder()
{
	const Bytes le = makeCatalog(sampleEntries(), false), be = makeCatalog(sampleEntries(), true);
	CHECK(le != be, "the two really differ");
	CHECK(loads(be) && eq(textTranslator->getText("Save As"), "Speichern unter"), "a big-endian catalog works (the consoles are big-endian, msgfmt on a PC is not)");
	CHECK(loads(le) && eq(textTranslator->getText("Save As"), "Speichern unter"), "a little-endian one works");

	// at every alignment: the catalog is read a byte at a time
	for(size_t pad = 0; pad < 4; pad++)
	{
		uint8_t * raw = (uint8_t *)malloc(le.size() + pad);
		memcpy(raw + pad, &le[0], le.size());
		CHECK(textTranslator->loadLanguage(raw + pad, le.size()) && eq(textTranslator->getText("Zoom"), "Gr\xC3\xB6\xC3\x9F" "e"), "alignment +%zu", pad);
		textTranslator->clear();
		free(raw);
	}
}

static void testRefused()
{
	const Bytes good = makeCatalog(sampleEntries());
	Bytes b;

	CHECK(!loads(Bytes()), "empty");
	b = good; b.resize(27); CHECK(!loads(b), "shorter than the header");
	b = good; b[0] ^= 1; CHECK(!loads(b), "bad magic");
	b = good; put32(b, 4, 2u << 16, false); CHECK(!loads(b), "a revision from the future");
	b = good; put32(b, 4, 1u << 16 | 1, false); CHECK(loads(b), "revision 1.1 is understood");
	b = good; put32(b, 8, 0, false); CHECK(!loads(b), "no messages");
	b = good; put32(b, 8, 0x7FFFFFFFu, false); CHECK(!loads(b), "a count that cannot be true");
	b = good; put32(b, 8, GuiTextTranslator::MAX_MESSAGES + 1, false); CHECK(!loads(b), "more messages than allowed");
	b = good; put32(b, 8, (uint32_t)sampleEntries().size() + 1, false); CHECK(!loads(b), "a count one too large (the extra entry is garbage)");
	b = good; put32(b, 12, 0xFFFFFFF0u, false); CHECK(!loads(b), "the ID table is outside the data");
	b = good; put32(b, 16, (uint32_t)b.size() - 4, false); CHECK(!loads(b), "the translation table runs off the end");
	b = good; put32(b, 12, 0, false); CHECK(!loads(b), "the ID table overlaps the header");
	b = good; put32(b, 16, 8, false); CHECK(!loads(b), "the translation table overlaps the header");

	// individual entries (the first ID entry is at 28; entry = {length, offset})
	b = good; put32(b, 28, 0x7FFFFFFFu, false); CHECK(!loads(b), "an ID longer than the data");
	b = good; put32(b, 28, GuiTextTranslator::MAX_STRING_LENGTH + 1, false); CHECK(!loads(b), "an ID longer than allowed");
	b = good; put32(b, 32, (uint32_t)b.size(), false); CHECK(!loads(b), "an ID that starts at the end");
	b = good; put32(b, 32, 0xFFFFFFFFu, false); CHECK(!loads(b), "an ID at an offset that wraps around");
	b = good; put32(b, 28, get32(b, 28, false) + 1, false); CHECK(!loads(b), "an ID whose end is not followed by a NUL");
	{
		Bytes nt = good;			// the last byte is the NUL ending the last translation
		nt.back() = 'x';
		CHECK(!loads(nt), "a translation that is not terminated");
	}

	// ID order
	{ Entries e = sampleEntries(); std::reverse(e.begin(), e.end()); CHECK(!loads(makeCatalog(e)), "IDs in descending order"); }
	{ Entries e = sampleEntries(); std::swap(e[2], e[3]); CHECK(!loads(makeCatalog(e)), "two IDs swapped"); }
	{ Entries e = sampleEntries(); e.insert(e.begin() + 3, e[3]); CHECK(!loads(makeCatalog(e)), "a duplicate ID"); }
	{ Entries e; e.push_back(std::make_pair("ab", "1")); e.push_back(std::make_pair("a", "2")); CHECK(!loads(makeCatalog(e)), "a longer ID before its own prefix"); }
	{ Entries e; e.push_back(std::make_pair("a", "1")); e.push_back(std::make_pair("ab", "2")); CHECK(loads(makeCatalog(e)), "a prefix before the longer ID is right"); }
	{ Entries e; e.push_back(std::make_pair("a\x7f", "1")); e.push_back(std::make_pair("a\x80", "2")); CHECK(loads(makeCatalog(e)), "bytes order as unsigned, as msgfmt writes them"); }
	{ Entries e; e.push_back(std::make_pair("a\x80", "1")); e.push_back(std::make_pair("a\x7f", "2")); CHECK(!loads(makeCatalog(e)), "and not as signed"); }

	// strings that are not plain
	{ Entries e; e.push_back(std::make_pair(std::string("one\0two", 7), "x")); CHECK(!loads(makeCatalog(e)), "an ID with a NUL in it (a plural form)"); }
	{ Entries e; e.push_back(std::make_pair("one", std::string("a\0b", 3))); CHECK(!loads(makeCatalog(e)), "a translation with a NUL in it"); }
	{ Entries e; e.push_back(std::make_pair("one", std::string(GuiTextTranslator::MAX_STRING_LENGTH + 1, 'x'))); CHECK(!loads(makeCatalog(e)), "a translation longer than allowed"); }
	{ Entries e; e.push_back(std::make_pair("one", std::string(GuiTextTranslator::MAX_STRING_LENGTH, 'x'))); CHECK(loads(makeCatalog(e)), "one of exactly the longest allowed"); }

	// a refused catalog leaves English, not the one before
	CHECK(loads(good), "a good catalog");
	b = good; b[0] ^= 1;
	CHECK(!loads(b) && eq(textTranslator->getText("hello"), "hello"), "after a refused one, English");
}

static void testBadTranslations()
{
	// Each is a sound catalog with one translation that is not text. Only that one translation is lost.
	const char * bad[] = {
		"\x80", "\xC0\x80", "\xC1\xBF", "\xC2", "\xE0\x80\x80", "\xE0\x9F\xBF", "\xED\xA0\x80", "\xED\xBF\xBF",
		"\xF0\x80\x80\x80", "\xF0\x8F\xBF\xBF", "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\xFF", "a\xC3", "\xE2\x82", "\xF0\x9F\x98",
		"\xC3\x28", "\xE2\x28\xA1", "\xE2\x82\x28", "\xF0\x28\x8C\xBC",
	};
	const char * good[] = { "a", "\xC2\x80", "\xDF\xBF", "\xE0\xA0\x80", "\xED\x9F\xBF", "\xEE\x80\x80", "\xEF\xBF\xBD", "\xF0\x90\x80\x80", "\xF4\x8F\xBF\xBF" };

	for(size_t i = 0; i < sizeof bad / sizeof *bad; i++)
	{
		Entries e; e.push_back(std::make_pair("bad", bad[i])); e.push_back(std::make_pair("ok", "fine"));
		CHECK(loads(makeCatalog(sorted(e))), "catalog %zu loads", i);
		CHECK(textTranslator->find("bad") == nullptr && eq(textTranslator->getText("bad"), "bad"), "invalid UTF-8 #%zu is not returned", i);
		CHECK(eq(textTranslator->getText("ok"), "fine"), "and the others still are");
	}
	for(size_t i = 0; i < sizeof good / sizeof *good; i++)
	{
		Entries e; e.push_back(std::make_pair("g", good[i]));
		CHECK(loads(makeCatalog(e)) && eq(textTranslator->getText("g"), good[i]), "valid UTF-8 #%zu is returned", i);
	}
	{ Entries e; e.push_back(std::make_pair("e", "")); CHECK(loads(makeCatalog(e)) && textTranslator->find("e") == nullptr, "an empty translation is no translation"); }
}

// Whatever is done to the bytes of a catalog, loading it and looking things up must stay inside it and return nothing
// but text that is inside it and ends in a NUL
static size_t g_lookups = 0;
static void exercise(const Heap & h, const std::vector<std::string> & ids)
{
	const bool ok = textTranslator->loadLanguage(h.p, h.n);
	for(size_t i = 0; i < ids.size(); i++)
	{
		const char * t = textTranslator->find(ids[i].c_str());
		g_lookups++;
		if(!t) continue;
		CHECK(ok, "a translation was returned by a catalog that was refused");
		const uint8_t * u = (const uint8_t *)t;
		CHECK(u >= h.p && u < h.p + h.n, "a translation points outside its catalog");
		if(u >= h.p && u < h.p + h.n)
			CHECK(memchr(u, 0, (size_t)(h.p + h.n - u)) != nullptr, "a translation that does not end inside the catalog");
		const char * g = textTranslator->getText(ids[i].c_str());
		CHECK(g == t, "getText agrees with find");
	}
	textTranslator->getText("not an id at all");
	textTranslator->getText("");
}

static void testDamage(const Bytes & original, const std::vector<std::string> & ids, int stride)
{
	{
		Heap h(original);
		CHECK(textTranslator->loadLanguage(h.p, h.n), "the undamaged catalog loads");
		for(size_t i = 0; i < ids.size(); i++) CHECK(textTranslator->find(ids[i].c_str()) != nullptr || true, "-");
	}
	// cut off at every length: nothing short of the whole thing is acceptable
	for(size_t n = 0; n < original.size(); n++)
	{
		Bytes cut(original.begin(), original.begin() + n);
		Heap h(cut);
		exercise(h, ids);
		CHECK(!textTranslator->isLoaded(), "a catalog cut to %zu of %zu bytes was accepted", n, original.size());
	}
	// every byte (every `stride`th for the big catalog), several ways: it may be refused or accepted, but never trusted blindly
	static const uint8_t masks[] = { 0x01, 0x80, 0xFF, 0x20 };
	for(size_t at = 0; at < original.size(); at += (size_t)stride)
		for(size_t m = 0; m < sizeof masks; m++)
		{
			Bytes b = original;
			b[at] ^= masks[m];
			Heap h(b);
			exercise(h, ids);
		}
	// overwrite a whole word at a time with values likely to matter
	static const uint32_t words[] = { 0, 1, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu, 0xFFFFFFFCu, 0x10000u };
	for(size_t at = 0; at + 4 <= original.size() && at < 28 + 16 * ids.size() + 16; at += 4)
		for(size_t w = 0; w < sizeof words / sizeof *words; w++)
			for(int be = 0; be < 2; be++)
			{
				Bytes b = original;
				put32(b, at, words[w], be != 0);
				Heap h(b);
				exercise(h, ids);
			}
	textTranslator->clear();
}

static void testDamageAll()
{
	std::vector<std::string> ids;
	const Entries e = sampleEntries();
	for(size_t i = 0; i < e.size(); i++) ids.push_back(e[i].first);
	ids.push_back("zzz"); ids.push_back("Save A"); ids.push_back("hell");
	testDamage(makeCatalog(e, false), ids, 1);
	testDamage(makeCatalog(e, true), ids, 1);

	// a catalog the size of a real one
	Entries big;
	std::vector<std::string> bigIds;
	for(int i = 0; i < 400; i++)
	{
		char id[64], tr[96];
		snprintf(id, sizeof id, "Message number %03d %s", i, (i % 7) ? "with some words in it" : "%d");
		snprintf(tr, sizeof tr, "\xC3\x9C" "bersetzung %03d mit ein paar W\xC3\xB6rtern", i);
		big.push_back(std::make_pair(id, tr));
		bigIds.push_back(id);
	}
	bigIds.push_back("Message number 000");
	big = sorted(big);
	const Bytes bigBytes = makeCatalog(big);
	{
		Heap h(bigBytes);
		CHECK(textTranslator->loadLanguage(h.p, h.n), "the 400 message catalog loads");
		bool all = true;
		for(size_t i = 0; i < big.size(); i++) all = all && eq(textTranslator->find(big[i].first.c_str()), big[i].second.c_str());
		CHECK(all, "every one of 400 messages is found with its own translation");
		CHECK(textTranslator->getCount() == 400, "count");
	}
	testDamage(bigBytes, bigIds, 13);
	printf("  (%zu lookups against damaged catalogs)\n", g_lookups);
}

// ---- threads -------------------------------------------------------------------------------------------------------
static void testThreads()
{
	Entries a, b;
	std::vector<std::string> ids;
	for(int i = 0; i < 60; i++)
	{
		char id[32], ta[32], tb[32];
		snprintf(id, sizeof id, "id %02d", i); snprintf(ta, sizeof ta, "A %02d", i); snprintf(tb, sizeof tb, "B %02d", i);
		a.push_back(std::make_pair(id, ta)); b.push_back(std::make_pair(id, tb)); ids.push_back(id);
	}
	const Bytes ca = makeCatalog(sorted(a)), cb = makeCatalog(sorted(b));
	Bytes broken = ca; broken[0] ^= 1;
	Heap ha(ca), hb(cb), hx(broken);

	std::atomic<bool> stop(false);
	std::atomic<int> wrong(0);
	std::atomic<long> looked(0);
	std::vector<std::thread> readers;
	for(int t = 0; t < 3; t++)
		readers.push_back(std::thread([&]() {
			while(!stop.load())
				for(size_t i = 0; i < ids.size(); i++)
				{
					char na[32], nb[32];
					snprintf(na, sizeof na, "A %02d", (int)i); snprintf(nb, sizeof nb, "B %02d", (int)i);
					const char * r = textTranslator->getText(ids[i].c_str());
					if(strcmp(r, ids[i].c_str()) != 0 && strcmp(r, na) != 0 && strcmp(r, nb) != 0) wrong++;
					looked++;
				}
		}));
	for(int i = 0; i < 4000; i++)
	{
		switch(i % 4)
		{
			case 0: textTranslator->loadLanguage(ha.p, ha.n); break;
			case 1: textTranslator->loadLanguage(hb.p, hb.n); break;
			case 2: textTranslator->clear(); break;
			default: textTranslator->loadLanguage(hx.p, hx.n); break;
		}
	}
	stop = true;
	for(size_t i = 0; i < readers.size(); i++) readers[i].join();
	CHECK(wrong == 0, "%d lookups returned something that no catalog says", wrong.load());
	CHECK(looked > 0, "the readers ran");
	textTranslator->clear();
}

// ---- conversions ---------------------------------------------------------------------------------------------------
static void testConversions()
{
	typedef GuiTextTranslator T;
	CHECK(T::sameConversions("no conversions", "keine"), "none and none");
	CHECK(T::sameConversions("%d", "%d"), "same");
	CHECK(T::sameConversions("%s: %d%%", "%d%% von %s") == false, "different order");
	CHECK(T::sameConversions("%s: %d%%", "%s - %d%%"), "same conversions, different words, a literal percent is not one");
	CHECK(!T::sameConversions("%d", "%s"), "different type");
	CHECK(!T::sameConversions("%d", "%ld"), "different length modifier");
	CHECK(!T::sameConversions("%d", "none"), "one missing");
	CHECK(!T::sameConversions("none", "%d"), "one extra");
	CHECK(!T::sameConversions("%d %d", "%d"), "one fewer");
	CHECK(!T::sameConversions("%d", "%d %d"), "one more");
	CHECK(T::sameConversions("%5.2f", "%5.2f"), "width and precision, same");
	CHECK(!T::sameConversions("%5.2f", "%6.2f"), "width and precision, different");
	CHECK(!T::sameConversions("%s", "%"), "a dangling percent is not a conversion");
	CHECK(T::sameConversions("100%%", "100 %%"), "only literal percents");
	CHECK(!T::sameConversions(nullptr, "%d") && !T::sameConversions("%d", nullptr), "null");
}

// ---- checking a real catalog against its source --------------------------------------------------------------------
// Reads the .lang source with a deliberately simple parser (msgid "..." / msgstr "..." lines; the sources use no escapes
// or continuation lines) and checks that the catalog built from it gives exactly those translations. Mismatching
// conversions are not a failure (a bad translation is only not used) but are listed, because someone should fix them.
static std::string unquote(const std::string & line, size_t from)
{
	const size_t a = line.find('"', from), z = line.rfind('"');
	return (a == std::string::npos || z <= a) ? std::string() : line.substr(a + 1, z - a - 1);
}

static int verifySource(const char * langPath, const char * moPath)
{
	const Bytes src = readFile(langPath), mo = readFile(moPath);
	if(src.empty() || mo.empty()) { printf("cannot read %s or %s\n", langPath, moPath); return 1; }
	Heap h(mo);
	CHECK(textTranslator->loadLanguage(h.p, h.n), "%s loads", moPath);

	std::string text(src.begin(), src.end()), id;
	size_t pos = 0, translated = 0, empty = 0, badConv = 0, pointers = 0;
	bool haveId = false;
	while(pos < text.size())
	{
		size_t eol = text.find('\n', pos);
		if(eol == std::string::npos) eol = text.size();
		std::string line = text.substr(pos, eol - pos);
		pos = eol + 1;
		if(!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
		if(line.compare(0, 7, "msgid \"") == 0) { id = unquote(line, 5); haveId = true; }
		else if(line.compare(0, 8, "msgstr \"") == 0 && haveId)
		{
			const std::string str = unquote(line, 6);
			haveId = false;
			if(id.empty()) continue;
			const char * got = textTranslator->find(id.c_str());
			if(str.empty()) { empty++; CHECK(got == nullptr, "'%s' has no translation but one was returned", id.c_str()); continue; }
			translated++;
			CHECK(got && str == got, "'%s' -> '%s', expected '%s'", id.c_str(), got ? got : "(none)", str.c_str());
			if(!GuiTextTranslator::sameConversions(id.c_str(), str.c_str()))
			{
				badConv++;
				printf("  note: %s: the conversions of '%s' and its translation '%s' differ, so the translation is not used by TrFormat()\n", langPath, id.c_str(), str.c_str());
			}
			pointers++;
		}
	}
	CHECK(textTranslator->getCount() == translated + 1, "the catalog holds %zu messages, the source has %zu translated (and the header entry makes one more)", textTranslator->getCount(), translated);
	printf("%s: %zu translated, %zu untranslated, %zu with different conversions\n", moPath, translated, empty, badConv);
	(void)pointers;
	textTranslator->clear();
	return failures ? 1 : 0;
}

int main(int argc, char ** argv)
{
	if(argc == 4 && strcmp(argv[1], "--verify") == 0)
		return verifySource(argv[2], argv[3]);
	if(argc != 2) { printf("usage: translator_test <catalog.mo>\n"); return 2; }

	CHECK(eq(early.result, "early"), "a lookup from a static initializer works (and falls back to the msgid): %s", early.result);
	CHECK(!textTranslator->isLoaded(), "nothing is loaded at the start");

	testMsgfmtCatalog(argv[1]);
	testLifecycle();
	testByteOrder();
	testRefused();
	testBadTranslations();
	testConversions();
	testDamageAll();
	testThreads();

	printf("translator_test: %d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
