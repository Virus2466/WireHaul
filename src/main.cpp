#include<iostream>
#include<iterator>
#include<fstream>
#include<cstdio>
#include "Bencode.hpp"

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



	std::cout << "\nas text: " << buf.substr(0, 40) << '\n';
	try
	{
		auto root = decode(buf);
		auto& d = *root.as<BencodeDict>();
		std::cout << "info dict bytes :" << d.at("info").raw.size() << '\n';

	} 
	catch (const ParseError& e)
	{
		std::cerr << "parse error : " << e.what() << '\n';
		return 1;
	}

	

}