#include "Bencode.hpp"
#include <charconv>
#include<string>

namespace wirehaul::bencode {
	

namespace {
		const int MaxDepth = 64;
		bool isDigit(char c ) 
		{
			return c >= '0' && c <= '9';
		}

		struct Parser
		{
			std::string_view s;
			size_t i = 0;

			[[noreturn]] void fail(const char* m) const
			{
				throw ParseError(std::string(m) + " at offset " + std::to_string(i));
			}

			char peek() const
			{
				if (i >= s.size())
				{
					fail(" Unexpected error - the end");
				}
				return s[i];
			}
			// Parses [-] digits  then consumes 'end'
			BencodeInt integer(char end)
			{
				size_t start = i;
				if (peek() == '-') ++i;
				size_t d = i;
				while (i < s.size() && isDigit(s[i])) ++i;
				if (i == d) fail(" expected digits ");
				if (s[d] == '0' && i - d > 1) fail(" leading zero ");
				if (s[start] == '-' && s[d] == '0') fail(" negative zero ");
				BencodeInt n;
				if (std::from_chars(s.data() + start, s.data() + i, n).ec != std::errc{}) fail("integer out of range");
				if (peek() != end) fail("bad terminator");
				++i;
				return n;
			}

			BencodeString string()
			{
				BencodeInt n = integer(':');
				if (n < 0 || static_cast<uint64_t>(n) > s.size() - i) fail("bad string length");
				auto r = s.substr(i, n);
				i += n;
				return r;
			}

			BencodeValue parse(int depth)
			{
				if (depth > MaxDepth) fail("nesting to deep");
				size_t start = i;
				BencodeValue out;
				char c = peek();
				if (c == 'i')
				{
					++i;
					out.value = integer('e');
				}
				else if (isDigit(c))
				{
					out.value = string();
				}
				else if (c == 'l')
				{
					++i;
					BencodeList l;
					while (peek() != 'e') l.push_back(parse(depth + 1));
					++i;
					out.value = std::move(l);
				}
				else if (c == 'd')
				{
					++i;
					BencodeDict d;
					while(peek() != 'e')
					{
						if (!isDigit(peek())) fail("dictionary key must be a string");
						auto k = string();
						if (!d.emplace(k, parse(depth + 1)).second) fail("duplicate key");
					}
					++i;
					out.value = std::move(d);
				}
				else
				{
					fail("Unexpected Byte");
				}
				out.raw = s.substr(start, i - start);
				return out;
			}
		};

} // inside namespace

BencodeValue decode(std::string_view data)
{
	Parser p{ data };
	auto v = p.parse(0);
	if (p.i != data.size())
	{
		p.fail("trailing data");
	}
	return v;
}


} // namespace bencode::wirehaul