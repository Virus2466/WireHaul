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
		return sha1(std::span)
	}





} // namespace wirehaul