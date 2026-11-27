#pragma once

#include<array>
#include<optional>
#include<cstdint>
#include<vector>


#include "Metainfo.hpp"
#include "Tracker.hpp" // for (ip + port)


namespace wirehaul {
	using PeerId = std::array<uint8_t, 20>;

	constexpr size_t HandshakeSize = 68;
	constexpr uint32_t MaxMessageSize = 2 * 1024 * 1024; //  we drop any peer that sends a bigger message

	using HandShake = std::array<uint8_t, HandshakeSize>;

	// message after the handshake
	struct Message {
		bool keepAlive = false;
		uint8_t id = 0;
		std::vector<uint8_t>payload;

	};

	struct PeerSessionResult {
		PeerId remotePeerId{};
		std::vector<uint8_t>bitfield;
		std::vector<uint8_t>messageIds;
	};

	// the first 68 bytes we send to say the first helo.
	HandShake buildHandShake(const Sha1Digest& infoHash, const PeerId& myId);

	std::optional<PeerId>parseHandshake(const HandShake& hello, const Sha1Digest& expectedinfoHash);

	const char* messageName(uint8_t id);

	size_t countPieces(const std::vector<uint8_t>& bitfield, size_t totalPieces);

	PeerSessionResult runPeerSession(const Peer& peer, const MetaInfo& m, const PeerId& myId);

	

} // namespace wirehaul