#include <iostream>
#include <string>
#include <string_view>
#include "Bencode.hpp"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void enableAnsi() {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD m;
	if (GetConsoleMode(h, &m)) SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#else
static void enableAnsi() {}
#endif

using namespace wirehaul::bencode;
using namespace std::string_view_literals;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define RESET "\033[0m"

static int total = 0, failed = 0;

#define CHECK(c) do { \
	++total; \
	if (c) std::cout << GREEN << total << " passed" << RESET << '\n'; \
	else { ++failed; std::cout << RED << total << " FAILED (line " << __LINE__ << "): " #c << RESET << '\n'; } \
} while (0)

static bool bad(std::string_view s) {
	try { decode(s); }
	catch (const ParseError&) { return true; }
	return false;
}

int main() {
	enableAnsi();

	// valid input
	CHECK(*decode("i-42e").as<BencodeInt>() == -42);
	CHECK(*decode("i0e").as<BencodeInt>() == 0);
	CHECK(*decode("4:spam").as<BencodeString>() == "spam");
	CHECK(decode("0:").as<BencodeString>()->empty());
	CHECK(decode("le").as<BencodeList>()->empty());
	CHECK(decode("de").as<BencodeDict>()->empty());
	CHECK(decode("l1:ai1ee").as<BencodeList>()->size() == 2);
	CHECK(decode("d3:cow3:mooe").as<BencodeDict>()->at("cow").as<BencodeString>()->size() == 3);

	// binary-safe string (embedded NUL)
	CHECK(decode("3:a\0b"sv).as<BencodeString>()->size() == 3);

	// raw span of a nested value (what the info-hash depends on)
	auto v = decode("d4:infod1:ai1eee"sv);
	CHECK(v.as<BencodeDict>()->at("info").raw == "d1:ai1ee");

	// invalid input must throw
	for (auto s : { "", "i-0e", "i03e", "ie", "i5", "5:abc", "-1:x", "3:", "l", "d1:a",
					"di1e1:ae", "d1:a1:a1:a1:be", "i1ex", "i9999999999999999999e" })
		CHECK(bad(s));
	CHECK(bad(std::string(100, 'l') + std::string(100, 'e'))); // depth limit

	if (failed) std::cout << RED << failed << " of " << total << " tests FAILED" << RESET << '\n';
	else        std::cout << GREEN << "all " << total << " tests passed" << RESET << '\n';
	return failed;
}