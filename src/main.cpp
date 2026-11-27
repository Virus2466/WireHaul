#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "Metainfo.hpp"
#include "PeerWire.hpp"
#include "Tracker.hpp"


void printTorrentInfo(const wirehaul::MetaInfo& m) {
	std::cout << "name:         " << m.name
		<< "\nsize:         " << m.totalLength
		<< "\npiece length: " << m.pieceLength
		<< "\npieces:       " << m.pieceHashes.size()
		<< "\nfiles:        " << m.files.size()
		<< "\ninfohash:     " << wirehaul::toHex(m.infoHash) << '\n';
}


void tryPeers(const wirehaul::AnnounceResult& r, const wirehaul::MetaInfo& m, const wirehaul::PeerId& myId) {
	for (size_t i = 0; i < r.peers.size() && i < 10; ++i) {
		const auto& p = r.peers[i];
		try {
			auto s = wirehaul::runPeerSession(p, m, myId);
			std::cout << p.ip << ':' << p.port << " ok, has "
				<< wirehaul::countPieces(s.bitfield, m.pieceHashes.size())
				<< " of " << m.pieceHashes.size() << " pieces\n";
			for (auto id : s.messageIds) std::cout << "  got: " << wirehaul::messageName(id) << '\n';
			return;   // one working peer is enough for now
		}
		catch (const std::exception& e) {
			std::cout << p.ip << ':' << p.port << " failed: " << e.what() << '\n';
		}
	}
}



int main(int argc, char** argv) {
	if (argc < 2) { std::cerr << "usage: wirehaul <file.torrent>\n"; return 1; }

	std::ifstream f(argv[1], std::ios::binary);
	if (!f) { std::cerr << "cannot open " << argv[1] << '\n'; return 1; }
	std::string buf((std::istreambuf_iterator<char>(f)), {});

	try {
		auto m = wirehaul::parseMetainfo(buf);
		printTorrentInfo(m);

		wirehaul::AnnounceParams ap;
		ap.peerId = wirehaul::makePeerId();
		ap.left = m.totalLength;

		for (auto& t : m.trackers) {
			if (t.rfind("http", 0) != 0) continue;   // to:do udp support
			try {
				auto r = wirehaul::announce(t, m, ap);
				std::cout << t << ": " << r.peers.size() << " peers, next announce in " << r.interval << "s\n";

				tryPeers(r, m, ap.peerId);

				ap.event = wirehaul::AnnounceEvent::Stopped;   
				try { wirehaul::announce(t, m, ap); }
				catch (...) {}
				break;                                         // one working tracker is enough for now
			}
			catch (const std::exception& e) {
				std::cerr << t << ": " << e.what() << '\n';
			}
		}
	}
	catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << '\n';
		return 1;
	}
}