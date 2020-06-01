#ifndef SHTTP_SERVER_REQUEST_H
#define SHTTP_SERVER_REQUEST_H

#include <string>

#include "headercontainer.h"
#include "method.h"

struct UriHeader 
{
		std::string method;
		std::string uri;
		std::string protocol;
};

struct RequestHeader: public HeaderContainer
{
	UriHeader uri;
	Method method;
	std::string body;

	explicit RequestHeader(const UriHeader& uri_header) :
		uri(uri_header),
		method(get_method(uri_header.method))
	{ }
};

#endif
