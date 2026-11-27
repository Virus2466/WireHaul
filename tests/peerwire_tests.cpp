#include <asio.hpp>
#include <iostream>
#include <thread>

#include "PeerWire.hpp"


using namespace wirehaul;
using asio::ip::tcp;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define RESET "\033[0m"
static int total = 0, failed = 0;


#define CHECK(c) do { ++total; \
	if (c) std::cout << GREEN << total << " passed" << RESET << '\n'; \
	else { ++failed; std::cout << RED << total << " FAILED (line " << __LINE__ << "): " #c << RESET << '\n'; } } while (0)

// this starts a fake peer on this computer: it answers the hello and then sends a bitfield
static std::thread startFakePeer(asio::io_context& ctx, tcp::acceptor& acc, const Sha1Digest& hash, size_t bitfieldBytes) {
	return std::thread([&ctx, &acc, hash, bitfieldBytes] {
		tcp::socket s(ctx);
		acc.accept(s);
		HandShake in{};
		asio::read(s, asio::buffer(in));
		PeerId theirId{};
		theirId.fill('P');
		asio::write(s, asio::buffer(buildHandShake(hash, theirId)));
		std::vector<uint8_t> msg = { 0, 0, 0, uint8_t(1 + bitfieldBytes), 5 };   // length, id 5 = bitfield
		msg.insert(msg.end(), bitfieldBytes, 0xFF);
		asio::write(s, asio::buffer(msg));
		});
}

int main() {
	Sha1Digest hash{}; hash.fill(0x11);
	PeerId me{}; me.fill('M');

	// hello message layout
	HandShake h = buildHandShake(hash, me);
	CHECK(h[0] == 19);
	CHECK(h[1] == 'B' && h[19] == 'l');
	CHECK(h[28] == 0x11 && h[47] == 0x11);
	CHECK(h[48] == 'M' && h[67] == 'M');
	CHECK(parseHandshake(h, hash).has_value());
	Sha1Digest other{}; other.fill(0x22);
	CHECK(!parseHandshake(h, other).has_value());
	h[5] = 'X';
	CHECK(!parseHandshake(h, hash).has_value());

	// bitfield counting: 10 pieces, bits 1111 0000 11xx xxxx -> 6 pieces
	CHECK(countPieces({ 0xF0, 0xC0 }, 10) == 6);

	// full talk with a fake peer on this computer
	asio::io_context ctx;
	tcp::acceptor acc(ctx, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
	uint16_t port = acc.local_endpoint().port();

	MetaInfo m;
	m.infoHash = hash;
	m.pieceHashes.resize(16);                    // 16 pieces -> bitfield is 2 bytes
	auto fake = startFakePeer(ctx, acc, hash, 2);
	try {
		auto r = runPeerSession(Peer{ "127.0.0.1", port }, m, me);
		CHECK(r.remotePeerId[0] == 'P');
		CHECK(r.bitfield.size() == 2);
		CHECK(countPieces(r.bitfield, 16) == 16);
	}
	catch (const std::exception& e) { std::cout << RED << "session threw: " << e.what() << RESET << '\n'; ++failed; }
	fake.join();

	// a peer that sends the wrong size bitfield must be rejected
	asio::io_context ctx2;
	tcp::acceptor acc2(ctx2, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 0));
	auto fake2 = startFakePeer(ctx2, acc2, hash, 5);
	bool rejected = false;
	try { runPeerSession(Peer{ "127.0.0.1", acc2.local_endpoint().port() }, m, me); }
	catch (const std::exception&) { rejected = true; }
	CHECK(rejected);
	fake2.join();

	if (failed) std::cout << RED << failed << " of " << total << " tests FAILED" << RESET << '\n';
	else        std::cout << GREEN << "all " << total << " tests passed" << RESET << '\n';
	return failed;
}