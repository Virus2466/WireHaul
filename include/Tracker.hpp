#pragma once

#include<array>
#include<cstdint>
#include<span>
#include<string>
#include<string_view>
#include<vector>

#include "Metainfo.hpp"

namespace wirehaul {
	struct Peer {
		std::string ip;
		uint64_t port;
	};

	struct AnnounceResult {
		uint32_t interval; // seconds until the next announce 
		std::vector<Peer>peers;
	};

	enum class AnnounceEvent {
		None,
		Started,
		Completed,
		Stopped
	};

	struct AnnounceParams {
		std::array<uint8_t, 20>peerId{};
		uint16_t port = 6881;
		uint64_t uploaded = 0, downloaded = 0, left = 0;
		AnnounceEvent event = AnnounceEvent::Started;
	};

	std::array<uint8_t, 20>makePeerId();

	// HTTPS announce , throws std::runtime_error ( network ) 
	AnnounceResult announce(const std::string& trackerUrl, const MetaInfo& m, const AnnounceParams& p);

	// exposed for testing purpose
	std::string urlEncode(std::span<const uint8_t>bytes);
	AnnounceResult parseAnnounceResponse(std::string_view body);


} // namespace wirehaul