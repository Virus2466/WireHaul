#include<iostream>
#include<iterator>
#include<fstream>
#include<cstdio>


#include "Bencode.hpp"
#include "Metainfo.hpp"
#include "Tracker.hpp"

using namespace wirehaul::bencode;




int main(int argc , char** argv) 
{
	if (argc < 2) { std::cerr << "usage: wirehaul <file.torrent>\n"; return 1; }
	std::ifstream f(argv[1], std::ios::binary);
	if (!f) { std::cerr << "cannot open " << argv[1] << '\n'; return 1; }

	std::string buf((std::istreambuf_iterator<char>(f)), {});


	// for checking torrent working.
	/*std::cout << "size: " << buf.size() << "\nfirst bytes: ";
	for (size_t k = 0; k < buf.size() && k < 16; ++k)
		std::printf("%02x ", (unsigned char)buf[k]);*/



	std::cout << "\n" << buf.substr(0, 40) << '\n';
	try
	{
		/*auto root = decode(buf);
		auto& d = *root.as<BencodeDict>();
		std::cout << "info dict bytes :" << d.at("info").raw.size() << '\n';*/
		auto m = wirehaul::parseMetainfo(buf);
		std::cout << "name:         " << m.name
			<< "\nsize:         " << m.totalLength
			<< "\npiece length: " << m.pieceLength
			<< "\npieces:       " << m.pieceHashes.size()
			<< "\nfiles:        " << m.files.size()
			<< "\ninfohash:     " << wirehaul::toHex(m.infoHash) << '\n';

		wirehaul::AnnounceParams ap;
		ap.peerId = wirehaul::makePeerId();
		ap.left = m.totalLength;

		std::cerr << "trackers: " << m.trackers.size() << '\n';
		for (auto& t : m.trackers) {
			if (t.rfind("http" , 0) != 0) { continue;}
			try {
				auto r = wirehaul::announce(t, m, ap);
				std::cout << t << ": " << r.peers.size() << " peers, next announce in " << r.interval << "s\n";
				for (size_t i = 0; i < r.peers.size() && i < 5; ++i) {
					std::cout << " " << r.peers[i].ip << ':' << r.peers[i].port << '\n';
				}

				ap.event = wirehaul::AnnounceEvent::Stopped; // deregister , since we aren't Listening yet ( work on this later)
				try { wirehaul::announce(t, m, ap); } catch(...){}
				break;
			} catch(const std::exception& e){
				std::cerr << t << ": " << e.what() << '\n';
			}
		}
	} 
	catch (const wirehaul::bencode::ParseError& e){
		std::cerr << "parse error : " << e.what() << '\n';
		return 1;
	}

	

}