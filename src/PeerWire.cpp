#include<asio.hpp>
#include<chrono>
#include<cstring>
#include<exception>
#include<stdexcept>

#include "PeerWire.hpp"


namespace wirehaul {
	namespace {
		using asio::awaitable;
		using asio::use_awaitable;
		using asio::ip::tcp;

		constexpr char kProtocol[] = "BitTorrent protocol"; // part of every message
		constexpr int kSecondsPerPeer = 10;
		constexpr int kMaxMessagesToRead = 10;

		// turns 4-bytes(big-endian as sent to the network)into normal number
		uint32_t readU32(const uint8_t* p) {

			return uint32_t(p[0] << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2] << 8 | uint32_t(p[3])));
		}

		awaitable<Message> readMessage(tcp::socket& sock) {

			uint8_t lenBuf[4];
			co_await asio::async_read(sock, asio::buffer(lenBuf), use_awaitable);
			uint32_t len = readU32(lenBuf);


			Message msg;
			if (len == 0) {
				msg.keepAlive = true;
				co_return msg;
			}
			
			if (len > MaxMessageSize) {
				throw std::runtime_error("peer sent a message with wrong size.");
			}

			std::vector<uint8_t>body(len);
			co_await asio::async_read(sock, asio::buffer(body), use_awaitable);
			msg.id = body[0];
			msg.payload.assign(body.begin() + 1, body.end());
			co_return msg;
		}

		// reads the first message the from the one peer : connect , hello 
		awaitable<PeerSessionResult> talkToPeer(const Peer& peer, const MetaInfo& m, const PeerId& myId) {

			auto ex = co_await asio::this_coro::executor;
			tcp::socket sock(ex);

		// watchdog : if this takes too long to respond , close the socket so any waiting fails
			asio::steady_timer watchdog(ex);
			watchdog.expires_after(std::chrono::seconds(kSecondsPerPeer));
			watchdog.async_wait([&sock](const asio::error_code& ec) {
				if (!ec) { asio::error_code ignored; sock.close(ignored); };
				});

			// connect (make_address throws if the ip text is not a real address)
			auto addr = asio::ip::make_address(peer.ip);
			co_await sock.async_connect(tcp::endpoint(addr, peer.port), use_awaitable);

			// send our hello :)
			HandShake hello = buildHandShake(m.infoHash, myId);
			co_await asio::async_write(sock, asio::buffer(hello), use_awaitable);

			HandShake reply{};
			co_await asio::async_read(sock, asio::buffer(reply.data(), 1), use_awaitable);
			if (reply[0] != 19) {
				throw std::runtime_error("peer is not speaking the BitTorrent protocol");
			}
			co_await asio::async_read(sock, asio::buffer(reply.data() + 1, HandshakeSize - 1), use_awaitable);

			auto remoteId = parseHandshake(reply, m.infoHash);
			if (!remoteId) {
				throw std::runtime_error("peer hello is for a different torrent");
			}

			PeerSessionResult result;
			result.remotePeerId = *remoteId;

			// read a few messages , stop when we get the bitfield
			size_t expectedBitfieldBytes = (m.pieceHashes.size() + 7) / 8;
			for (int i = 0; i < kMaxMessagesToRead; ++i) {
				Message msg = co_await readMessage(sock);
				if (msg.keepAlive) { continue; }
				result.messageIds.push_back(msg.id);
				if (msg.id == 5) {
					if (msg.payload.size() != expectedBitfieldBytes) {
						throw std::runtime_error("peer sent a bitfield with the wrong size");
					}
					result.bitfield = std::move(msg.payload);
					break;
				}
			}
			watchdog.cancel();
			co_return result;


		}


	} // inner namespace
	


	HandShake buildHandShake(const Sha1Digest& infoHash, const PeerId& myId) {

		HandShake h{};
		h[0] = 19;
		std::memcpy(h.data() + 1, kProtocol, 19); // protocol name
		std::memcpy(h.data() + 28, infoHash.data(), 20); // which torrent we want
		std::memcpy(h.data() + 48, myId.data(), 20); // who we are
		return h;
	}

	std::optional<PeerId> parseHandshake(const HandShake& hello, const Sha1Digest& expectedinfoHash){

		if (hello[0] != 19) { return std::nullopt; }
		if (std::memcmp(hello.data() + 1, kProtocol, 19) != 0) { return std::nullopt; }
		if (std::memcmp(hello.data() + 28, expectedinfoHash.data(), 20) != 0) { return std::nullopt; }
		PeerId id;
		std::memcpy(id.data(), hello.data() + 48, 20);
		return id;
	}

	const char* messageName(uint8_t id)
	{
		switch (id) {
			case 0: return "choke";
			case 1: return "unchoke";
			case 2: return "interested";
			case 3: return "not interested";
			case 4: return "have";
			case 5: return "bitfield";
			case 6: return "request";
			case 7: return "piece";
			case 8: return "cancel";
			case 20: return "extended";
			default: return "unknown";
		}
	}

	size_t countPieces(const std::vector<uint8_t>& bitfield, size_t totalPieces) {

		size_t count = 0;
		for (size_t i = 0; i < totalPieces && i / 8 < bitfield.size(); ++i) {
			if (bitfield[i / 8] & (0x80 >> (i % 8))) { ++count; }
		}
		return count;
	}

	PeerSessionResult runPeerSession(const Peer& peer, const MetaInfo& m, const PeerId& myId) {
		
		asio::io_context ctx;
		PeerSessionResult result;
		std::exception_ptr error;

		asio::co_spawn(ctx, talkToPeer(peer, m, myId),
			[&](std::exception_ptr e, PeerSessionResult r) {
			error = e;
			result = std::move(r);});
		ctx.run();

		if (error) { std::rethrow_exception(error); }
		return result;
	}

} // namespace wirehaul
