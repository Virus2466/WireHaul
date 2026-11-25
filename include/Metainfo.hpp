#pragma once
#include<cstdint>
#include<string>
#include<string_view>
#include<vector>


#include "Sha1.hpp"


using u64 = uint64_t;


namespace wirehaul {

	struct FileEntry {
		std::string path; // relative, '/'-separated, already validated
		u64 length;
		
	};

	struct MetaInfo {
		std::string name;
		std::vector<std::string>trackers;
		u64 pieceLength = 0;
		std::vector<Sha1Digest>pieceHashes;
		std::vector<FileEntry>files;
		u64 totalLength = 0;
		Sha1Digest infoHash{};
	};

	// Owning result: `bytes` may be freed afterwards. Throws bencode::ParseError.
	MetaInfo parseMetainfo(std::string_view bytes);


} // namespace

