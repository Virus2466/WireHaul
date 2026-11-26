#include<algorithm>
#include<cstring>
#include<random>
#include<stdexcept>
#include<curl/curl.h>



#include "Bencode.hpp"
#include "Tracker.hpp"


// ** to-do : tidy up the functions 

namespace wirehaul {
	using namespace bencode;

	namespace {
		constexpr size_t MaxResponse = 1 << 20; // a hostile tracker can't make us buffer more than 1MiB

		size_t onData(char* p, size_t size, size_t n, void* out) {
			auto* s = static_cast<std::string*>(out);
			if (s->size() + size * n > MaxResponse) {
				return 0;
			}
			s->append(p, size * n);
			return size * n;

		}

	} // inner namespace





	// percent encoding : info_hash and peer_id are raw binary , 
	// so every byte that isn't A-z , a-z 0-9 , - , _ ~ becomes %XX urlEncodeDoes That.
	
	std::string urlEncode(std::span<const uint8_t> bytes) {
		static constexpr char hex[] = "0123456789ABCDEF";
		std::string s;
		for (uint8_t c : bytes) {
			bool plain = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
				|| c == '-' || c == '.' || c == '_' || c == '~';

			if (plain) { s += char(c); }
			else {
				s += '%';
				s += hex[c >> 4];
				s += hex[c & 15];
			}
		}
		return s;
	}


	//-WH0001- plus 12 random bytes follows the common client-code-and-version convention, so trackers and other clients can identify WireHaul.
	std::array<uint8_t, 20> makePeerId() {
		std::array<uint8_t, 20> id{};
		std::memcpy(id.data(), "-WH0001-", 8); // client code + version
		std::random_device rd;
		for (size_t i = 8; i < id.size(); ++i) {
			id[i] = uint8_t(rd());
		}
		return id;
	}

	

	AnnounceResult parseAnnounceResponse(std::string_view body) {
		auto root = decode(body);
		auto* d = root.as<BencodeDict>();
		if (!d) {
			throw ParseError("tracker reply is not dict");
		}

		if (auto it = d->find("failure reason"); it != d->end()) {
			if (auto* s = it->second.as<BencodeString>()) {
				throw ParseError("tracker : " + std::string(*s));
			}
		}

		AnnounceResult r;
		if (auto it = d->find("interval"); it != d->end()) {
			if (auto* n = it->second.as<BencodeInt>(); n&&* n > 0){
				r.interval = uint32_t(std::min<BencodeInt>(*n, 86400));
			}
		}

		auto it = d->find("peers");
		if(it == d->end()) { return r; }

		if (auto* s = it->second.as<BencodeString>()) {

			auto* b = reinterpret_cast<const uint8_t*>(s->data());
			for (size_t i = 0; i + 6 <= s->size(); i += 6, b += 6) {
				r.peers.push_back({ std::to_string(b[0]) + '.' + std::to_string(b[1]) + '.'
								  + std::to_string(b[2]) + '.' + std::to_string(b[3]),
									uint16_t(b[4] << 8 | b[5]) });
			}

		}
		else if (auto* l = it->second.as<BencodeList>()) {
			for (auto& e : *l) {
				auto* pd = e.as<BencodeDict>();
				if (!pd) { continue; }
				auto iPIt = pd->find("ip"), portIt = pd->find("port");
				if (iPIt == pd->end() || portIt == pd->end()) { continue; }
				auto* ip = iPIt->second.as<BencodeString>();
				auto* port = portIt->second.as<BencodeInt>();
				if (ip && port && *port > 0 && *port < 65536) {
					r.peers.push_back({ std::string(*ip) , uint16_t(*port) });
				}
			}
		}
		
		return r;
	}


	AnnounceResult announce(const std::string& url, const MetaInfo& m, const AnnounceParams& p){
		static const char* const events[] = { "" , "started", "completed" , "stopped" };
		
		std::string full = url + (url.find('?') == std::string::npos ? '?' : '&')
			+ "info_hash=" + urlEncode(m.infoHash)
			+ "&peer_id=" + urlEncode(p.peerId)
			+ "&port=" + std::to_string(p.port)
			+ "&uploaded=" + std::to_string(p.uploaded)
			+ "&downloaded=" + std::to_string(p.downloaded)
			+ "&left=" + std::to_string(p.left)
			+ "&compact=1";


		if (p.event != AnnounceEvent::None) {
			full += std::string("&event=") + events[int(p.event)];
		}

		static const bool once = (curl_global_init(CURL_GLOBAL_DEFAULT), true);
		(void)once;

		CURL* c = curl_easy_init();
		if (!c) {
			throw std::runtime_error("curl init failed");
		}

		std::string body;
		char err[CURL_ERROR_SIZE] = {};
		curl_easy_setopt(c, CURLOPT_URL, full.c_str());
		curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, onData);
		curl_easy_setopt(c, CURLOPT_WRITEDATA, &body);
		curl_easy_setopt(c, CURLOPT_ERRORBUFFER, err);
		curl_easy_setopt(c, CURLOPT_TIMEOUT, 15L);
		curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(c, CURLOPT_USERAGENT, "WireHaul/0.1");

		CURLcode rc = curl_easy_perform(c);
		long status = 0;
		curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &status);
		curl_easy_cleanup(c);

		if (rc != CURLE_OK) {
			throw std::runtime_error(std::string("request failed :") + (err[0] ? err : curl_easy_strerror(rc)));
		}
		if (status != 200) {
			throw std::runtime_error("tracker returned HTTP  " + std::to_string(status));
		}

		return parseAnnounceResponse(body);


	}




} // namepsace wirehaul
