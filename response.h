#ifndef SHTTP_SERVER_RESPONSE_H
#define SHTTP_SERVER_RESPONSE_H

#include <string>
#include <unordered_map>
#include <utility>

struct HandlerResponse: public HeaderContainer
{
	std::string code;
	std::string desc;
	std::string body;

  HandlerResponse(std::string code,
                  std::string code_desc,
                  std::string body = std::string()):code{std::move(code)}, desc{std::move(code_desc)}, body{std::move(body)}
    { }
};

struct Response 
{
	RequestHeader request;
	HandlerResponse response;

  friend auto & operator<<(std::ostream & stream, const Response &r) {
    stream << r.request.uri.protocol <<  " " << r.response.code << " " << r.response.desc << "\n";
    return r.response.to_stream(stream);
  }
};

#endif
