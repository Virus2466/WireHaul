#pragma once

#include<iostream>
#include<array>
#include<span>
#include<string>
#include<string_view>
#include<cstdint>


namespace wirehaul {
	using Sha1Digest = std::array<uint8_t, 20>;

	Sha1Digest sha1(std::span<const uint8_t> data);

	inline Sha1Digest sha1(std::string_view s) {
		return sha1(std::span(reinterpret_cast<const uint8_t*>(s.data()), s.size()));
	}

	inline std::string toHex(const Sha1Digest& d) {
		static constexpr char h[] = "0123456789abcdef";
		std::string s;
		for (uint8_t b : d) {
			s += h[b >> 4];
			s += h[b & 15];
		}
		return s;
	}

} // namespace wirehaul