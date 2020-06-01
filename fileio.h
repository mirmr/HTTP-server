#ifndef SHTTP_SERVER__FILEIO_H_
#define SHTTP_SERVER__FILEIO_H_

#include <optional>
#include <fstream>
#include <algorithm>

#include <sys/stat.h>

#include "base.h"

enum FileType {
  REGULAR,
  DIRECTORY,
  UNKNOWN
};

struct FileStatus {
  bool is_exists;
  FileType file_type;
};

inline auto append_path(const std::string &base, const std::string &a)
{
  std::ostringstream ss;
  ss << base;
  if (!base.empty() && base.back() != '/')
    ss << "/";
   ss << (!a.empty() && a.back() == '/' ? a.substr(1) : a);
   return ss.str();
}

inline FileStatus file_status(const std::string &p)
{
  struct stat status{};
  if (stat(p.c_str(), &status) != 0)
    return { false, FileType::REGULAR };
  switch (status.st_mode & S_IFMT)
  {
    case S_IFDIR: return {true, FileType::DIRECTORY};
    case S_IFREG: return {true, FileType::REGULAR};
    default: return {true, FileType::UNKNOWN};
  }
}

inline std::string file_extension(const std::string &p)
{
  if (auto ext_begin = std::find(p.rbegin(), p.rend(), '.'); ext_begin != p.rend())
  {
    return p.substr(p.rend() - ext_begin - 1);
  }
  else
  {
    return std::string();
  }
}

inline auto get_path(const std::string &p)
{
  return append_path(get_base_dir(), p);
}

inline auto read_file(const std::string &p)
{
  std::ifstream file(p);
  if (file.is_open())
  {
    std::stringstream buffer;
    buffer << file.rdbuf();
    return std::optional(buffer.str());
  }
  return std::optional<std::string>();
}

#endif//SHTTP_SERVER__FILEIO_H_
