#ifndef SHTTP_SERVER__HEADERCONTAINER_H_
#define SHTTP_SERVER__HEADERCONTAINER_H_

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

typedef std::unordered_map<std::string, std::string> HeadersMap;

struct HeaderContainer {
  std::vector<std::string> header_keys;
  HeadersMap headers;

  void add_header(const std::string &key, const std::string &value)
  {
    headers.insert_or_assign(key, value);
    header_keys.push_back(key);
  }

  bool has_header(const std::string &name) const
  {
    return std::find(header_keys.begin(), header_keys.end(), name) != header_keys.end();
  }

  const std::string & operator[](const std::string &key) const
  {
    return headers.at(key);
  }

  auto & to_stream(std::ostream & stream) const {
    for (const auto & [key, value] : headers) {
      stream << key << ": " << value << "\n";
    }
    return stream;
  }

};

#endif//SHTTP_SERVER__HEADERCONTAINER_H_
