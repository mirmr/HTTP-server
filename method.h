#ifndef SHTTP_SERVER_METHOD_H
#define SHTTP_SERVER_METHOD_H

#include <string>

enum Method {
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE,
	PATCH,
	INVALID
};

inline Method get_method(const std::string &m) 
{
  if (m == "GET") 	  return Method::GET;
  if (m == "POST") 	  return Method::POST;
  if (m == "HEAD") 	  return Method::HEAD;
  if (m == "PUT") 	  return Method::PUT;
  if (m == "DELETE")    return Method::DELETE;
  if (m == "CONNECT")   return Method::CONNECT;
  if (m == "OPTIONS")	  return Method::OPTIONS;
  if (m == "TRACE") 	  return Method::TRACE;
  if (m == "PATCH") 	  return Method::PATCH;
  return Method::INVALID;
}

inline bool should_read_body(Method m)
{
	return true;
}
#endif
