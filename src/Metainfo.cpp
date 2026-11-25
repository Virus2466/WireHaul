#include<algorithm>
#include<cstring>
#include<string>

#include "Bencode.hpp"
#include "Metainfo.hpp"

namespace wirehaul {
	using namespace bencode;
	namespace {
		[[noreturn]] void bad(const std::string& m) {
			throw ParseError(m);
		}

		// lookup gets the key value in the given meta info.
		const BencodeValue* lookup(const BencodeDict& d, std::string_view k) {
			auto it = d.find(k);
			return it == d.end() ? nullptr : &it->second;
		}

		template <class T> const T& need(const BencodeDict& d, std::string_view k) {
			auto* v = lookup(d, k);
			auto* p = v ? v->as<T>() : nullptr;
			if (!p) bad("missing or wrong-typed field " + std::string(k));
			return *p;
		}
		u64 needUint(const BencodeDict& d, std::string_view k) {
			auto n = need<BencodeInt>(d, k);
			if (n < 0) {
				bad("negative value");
			}
			return u64(n);
		}

		// Rejects Traversal and anything that cold escape the download folder.
		bool safeComponent(std::string_view c) {
			return !c.empty() && c != "." && c != ".."
				&& c.find_first_of("/\\:") == std::string_view::npos 
				&& c.find('\0') == std::string_view::npos;
		}

	} // inner namespace

	MetaInfo parseMetainfo(std::string_view bytes) {
		auto root = decode(bytes);
		auto* rd = root.as<BencodeDict>();

		if (!rd) {
			bad("Torrent is not a dict");
		}

		auto* iv = lookup(*rd, "info");
		auto* info = iv ? iv->as<BencodeDict>() : nullptr;
		if (!info) {
			bad("Missing info dict");
		}

		/*for (auto& [k, v] : *info) std::cerr << "info key: " << k << '\n';
		if (auto* p = lookup(*info, "pieces"))
			std::cerr << "pieces type index: " << p->value.index() << " raw size: " << p->raw.size() << '\n';*/


		MetaInfo m;
		m.infoHash = sha1(iv->raw);
		m.name = std::string(need<BencodeString>(*info, "name"));

		if (!safeComponent(m.name)) {
			bad("unsafe name");
		}

		m.pieceLength = needUint(*info, "piece length");
		if (m.pieceLength == 0) {
			bad("zero piece length");
		}

		auto pieces = need<BencodeString>(*info, "pieces");
		if (pieces.size() % 20) {
			bad("pieces size not a multiple of 20");
		}
		m.pieceHashes.resize(pieces.size() / 20);
		for (size_t i = 0; i < m.pieceHashes.size(); ++i) {
			std::memcpy(m.pieceHashes[i].data(), pieces.data() + i * 20, 20);
		}

		bool single = lookup(*info, "length"), multi = lookup(*info, "files");
		if (single == multi) {
			bad("need exactly one of length / files");
		}
		if (single) {
			m.files.push_back({ m.name , needUint(*info,"length") });
		}
		else {
			for (auto& fv : need<BencodeList>(*info, "files")) {
				auto* fd = fv.as<BencodeDict>();
				if (!fd) bad("bad file entry");
				std::string path = m.name;
				size_t parts = 0;
				for (auto& c : need<BencodeList>(*fd, "path")) {
					auto* s = c.as<BencodeString>();
					if (!s || !safeComponent(*s)) bad("unsafe path component");
					path += '/';
					path += *s;
					++parts;
				}
				if (!parts) bad("empty file path");
				m.files.push_back({ std::move(path) , needUint(*fd , "length") });
			}
			if (m.files.empty()) bad("no files");
		}

		for (auto& f : m.files) {
			if (f.length > UINT64_MAX - m.totalLength) bad("total size overflow");
			m.totalLength += f.length;
		}

		uint64_t expected = m.totalLength / m.pieceLength + (m.totalLength % m.pieceLength != 0);
		if (m.pieceHashes.size() != expected) bad("piece count does not match size");

		// for later purposes, this is a starter for DHT / magnet links
		auto add = [&](std::string_view u) {
			std::string s(u);
			if (!s.empty() && std::find(m.trackers.begin(), m.trackers.end(), s) == m.trackers.end()) {
				m.trackers.push_back(std::move(s));
			}

			};
		if (auto* a = lookup(*rd, "announce")) {
			if (auto* s = a->as<BencodeString>()) add(*s);
		}
		if (auto* al = lookup(*rd, "announce-list")) {
			if (auto* tiers = al->as<BencodeList>()) {
				for (auto& t : *tiers) {
					if (auto* tl = t.as<BencodeList>()) {
						for (auto& u : *tl) {
							if (auto* s = u.as<BencodeString>()) {
								add(*s);
							}
						}
					}
				}
			}
		}
		return m;

		
	}


} // namespace wirehaul