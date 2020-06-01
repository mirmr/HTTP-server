#ifndef SHTTP_SERVER_UTILS_H
#define SHTTP_SERVER_UTILS_H

#include <string>
#include <vector>
#include <algorithm>

inline auto consume(const std::string &from, char until)
{
	auto pos = from.find_first_of(until);
	if (pos == std::string::npos) 
	{
		return std::make_pair(from.substr(0), std::string());
	}
	return std::make_pair(from.substr(0, pos), from.substr(pos + 1));
}

inline std::vector<std::string> tokenize(const std::string &source, char del)
{
	std::string rest = source.substr(0);
	std::vector<std::string> tokens;
	do
	{
		auto [x, xs] = consume(rest, del);
		tokens.push_back(x);
		rest = xs;
	}
	while (!rest.empty());
	return tokens;
}

inline bool is_hex(char c) 
{
	return (c >= '0' && c <= '9') ||
					(c >= 'a' && c <= 'f') ||
					(c >= 'A' && c <= 'F');
}

inline std::string url_decode(const std::string &str)
{
	auto tokens = tokenize(str, '%');
	std::string decoded;
	for (const auto &token : tokens) 
	{
		if (token.size() >= 2)
		{
			auto num_str = token.substr(0, 2);
			auto is_num = std::all_of(num_str.begin(), num_str.end(), [](char c){ return is_hex(c); });
			if (is_num)
			{
				auto num = std::stoi(num_str, nullptr, 16);
				decoded += static_cast<char>(num);
				decoded += token.substr(2);
				continue;
			}
		}
		decoded += token;
	}
	return decoded;
}

#endif
