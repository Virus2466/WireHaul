#include "Sha1.hpp"
#include<cstring>


using u32 = uint32_t;
using u8 = uint8_t;
using u64 = uint64_t;

namespace wirehaul {
	namespace {
		constexpr u32 rol(u32 x, int n) {
			return (x << n) | (x >> (32 - n));
		}

		void block(u32 h[5], const u8* p) {
			u32 w[80];
			for (int i = 0; i < 16; ++i) {
				w[i] = u32(p[4 * i]) << 24 | u32(p[4 * i + 1]) << 16 | u32(p[4 * i + 2]) << 8 | p[4 * i + 3];
			}

			for (int i = 16; i < 80; ++i) {
				w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
			}

			u32 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

			for (int i = 0; i < 80; ++i) {
				u32 f, k;
				if (i < 20) { f = (b & c) | (-b & d); k = 0x5A827999;
				}
				else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1; }
				else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
				else { f = b ^ c ^ d;                               k = 0xCA62C1D6; }
				u32 t = rol(a, 5) + f + e + k + w[i];
				e = d; d = c; c = rol(b, 30); b = a; a = t;

			}
			h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
		}

	} // inner namespace

	Sha1Digest sha1(std::span<const u8>in) {
		u32 h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };

		size_t full = in.size() / 64;
		for (size_t i = 0; i < full; ++i) block(h, in.data() + i * 64);
		
		// padding  : 0x80 zeros , 64-bit big endian bit length
		u8 tail[128] = {};
		size_t rem = in.size() - full * 64;
		if (rem) std::memcpy(tail, in.data() + full * 64, rem);
		tail[rem] = 0x80;
		size_t len = rem < 56 ? 64 : 128;
		u64 bits = u64(in.size()) * 8;
		for (int i = 0; i < 8; ++i) tail[len - 1 - i] = u8(bits >> (8 * i));
		block(h, tail);
		if (len == 128) block(h, tail + 64);

		Sha1Digest out;
		for (int i = 0; i < 5; ++i) {
			for (int j = 0; j < 5; ++j) {
				out[4 * i + j] = u8(h[i] >> (24 - 8 * j));
			}
		}
		return out;
	}


} // namespace wirehaul