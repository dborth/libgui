// Host test of GuiTextTranslator: lookups, reloads, and what it does when its memory has been overwritten. It links the
// real GuiTextTranslator.cpp, so it does not use host_stubs.cpp (which stubs the translator for the widget tests).
#include "libgui/GuiTextTranslator.h"
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

// ---- a static that resolves text during static initialization -------------------------------------------------------
// The instance is defined the way GuiText.cpp defines the real one (keep the two in sync), AFTER a static that uses it:
// were the translator built by a dynamic initializer, `early` would run first and find nothing.
extern GuiTextTranslator * textTranslator;
struct Early { const char * result; Early() { result = textTranslator ? textTranslator->getText("early") : "NO TRANSLATOR"; } };
static Early early;

static union TranslatorStorage
{
	GuiTextTranslator translator;
	constexpr TranslatorStorage() : translator() {}	// does not compile unless the constructor is constexpr
	~TranslatorStorage() {}
} translatorStorage;
GuiTextTranslator * textTranslator = &translatorStorage.translator;

// ---- trap catching: fatalCorruption() ends in __builtin_trap(), which is SIGILL (x86) or SIGTRAP (others) -------------
static sigjmp_buf jump;
static void onTrap(int) { siglongjmp(jump, 1); }
static bool traps(void (*fn)())
{
	struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_handler = onTrap;
	sigaction(SIGILL, &sa, nullptr); sigaction(SIGTRAP, &sa, nullptr);
	if(sigsetjmp(jump, 1) == 0) { fn(); return false; }
	return true;
}
static void lookup() { (void)textTranslator->getText("hello"); }
static void lookupMissing() { (void)textTranslator->getText("not in the table"); }

int main()
{
	CHECK(strcmp(early.result, "early") == 0, "a lookup from a static initializer works (and falls back to the msgid): %s", early.result);

	const char * lang = "# comment\nmsgid \"hello\"\nmsgstr \"hola\"\nmsgid \"bye\"\nmsgstr \"adios\\n\"\nmsgid \"empty\"\nmsgstr \"\"\n";
	CHECK(textTranslator->loadLanguage(reinterpret_cast<const uint8_t *>(lang), strlen(lang)), "loads");
	CHECK(strcmp(textTranslator->getText("hello"), "hola") == 0, "translated");
	CHECK(strcmp(textTranslator->getText("bye"), "adios\n") == 0, "escapes are expanded");
	CHECK(strcmp(textTranslator->getText("empty"), "empty") == 0, "an empty msgstr falls back to the msgid");
	CHECK(strcmp(textTranslator->getText("unknown"), "unknown") == 0, "an unknown msgid comes back as it was");
	CHECK(textTranslator->getText(nullptr) == nullptr, "null in, null out");

	CHECK(textTranslator->loadLanguage(reinterpret_cast<const uint8_t *>(lang), strlen(lang)), "reloads (clear + rebuild)");
	CHECK(strcmp(textTranslator->getText("hello"), "hola") == 0, "still translated after a reload");
	CHECK(!textTranslator->loadLanguage(nullptr, 0), "an empty load reports failure");
	CHECK(strcmp(textTranslator->getText("hello"), "hello") == 0, "and leaves the table empty");
	CHECK(textTranslator->loadLanguage(reinterpret_cast<const uint8_t *>(lang), strlen(lang)), "loads again");

	// Layout (see the class): uint32_t magic, then the head pointer at the next pointer-aligned offset; a node is
	// { uint32_t id, char * msgstr, MSG * next }.
	void ** head = reinterpret_cast<void **>(reinterpret_cast<char *>(textTranslator) + sizeof(void *));
	uint32_t * magic = reinterpret_cast<uint32_t *>(textTranslator);

	// a head that is not an aligned pointer: the value from crash report #1153
	void * savedHead = *head;
	*head = reinterpret_cast<void *>(static_cast<uintptr_t>(0x85F8FD66u));
	CHECK(traps(lookup), "a misaligned list head stops the lookup");
	*head = savedHead;
	CHECK(!traps(lookup), "and the table is usable again once it is put back");

	// a node whose next pointer was overwritten: reached only by walking past the nodes before it
	void ** next = reinterpret_cast<void **>(reinterpret_cast<char *>(savedHead) + 2 * sizeof(void *));
	void * savedNext = *next;
	*next = reinterpret_cast<void *>(static_cast<uintptr_t>(0x85F8FD66u));
	CHECK(traps(lookupMissing), "a damaged next pointer stops the walk before it is followed");
	*next = savedNext;
	CHECK(!traps(lookupMissing), "undamaged again");

	// the object itself overwritten
	uint32_t savedMagic = *magic;
	*magic = 0x12345678u;
	CHECK(traps(lookup), "a wrong magic stops the lookup");
	*magic = savedMagic;
	CHECK(!traps(lookup), "restored");

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
