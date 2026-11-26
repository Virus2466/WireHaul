#include <iostream>
#include "Sha1.hpp"
using namespace wirehaul;

static int failed = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << __FILE__ << ':' << __LINE__ << " FAILED: " #c "\n"; ++failed; } } while (0)

int main() {
	CHECK(toHex(sha1("")) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
	CHECK(toHex(sha1("abc")) == "a9993e364706816aba3e25717850c26c9cd0d89d");
	CHECK(toHex(sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
	CHECK(toHex(sha1(std::string(1000000, 'a'))) == "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
	std::cout << (failed ? "FAILED\n" : "sha1 ok -- ALL TESTS PASSED\n");
	return failed;
}