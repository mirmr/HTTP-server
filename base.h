#ifndef SHTTP_SERVER__BASE_H_
#define SHTTP_SERVER__BASE_H_

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

inline auto get_date()
{
  std::time_t time = std::time(nullptr);
  std::tm *tm = std::localtime(&time);
  std::ostringstream date_stream;
  date_stream << std::put_time(tm, "%a, %d %b %Y %H:%M:%S GMT%Z");
  return date_stream.str();
}

inline std::string get_base_dir()
{
  return "../web/";
}

inline std::string get_server_id()
{
  return "SHTTP/1.0";
}

#endif//SHTTP_SERVER__BASE_H_
