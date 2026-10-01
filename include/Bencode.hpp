#pragma once 

#include<string>
#include<string_view>
#include<map>
#include<vector>
#include<variant>
#include<cstdint>
#include<stdexcept>

/*
		metainfo files keys
		announce - url of tracker 
		info - this maps to dict
		info -- 
		name -> UTF-8
		piece -> number of bytes 
		length --> length of file in bytes 
		path 

		Tracker get request keys
		peer_id,
		ip

	
*/


namespace wirehaul::bencode {
	// Bencode Data types

	struct BencodeValue;
	struct ParseError;

	using BencodeInt = int64_t;
	using BencodeString = std::string_view;
	using BencodeList = std::vector<BencodeValue>;
	using BencodeDict = std::map<std::string_view, BencodeValue>;
	
	struct BencodeValue {
		std::variant<BencodeInt, BencodeString, BencodeList, BencodeDict> value;
		std::string_view raw;

		template<class T> const T* as() const { return std::get_if<T>(&value); }
	};
	
	struct ParseError : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	BencodeValue decode(std::string_view data);
} // namespace end

