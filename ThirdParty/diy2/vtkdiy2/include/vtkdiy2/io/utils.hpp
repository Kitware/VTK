#ifndef DIY_IO_UTILS_HPP
#define DIY_IO_UTILS_HPP

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#include <share.h>
#else
#include <unistd.h>     // mkstemp() on Mac
#include <dirent.h>
#endif

#include <cstdio>       // remove()
#include <cstdlib>      // mkstemp() on Linux
#include <sys/stat.h>

#include "../constants.h" // for DIY_UNUSED

namespace diy
{
namespace io
{
namespace utils
{
  namespace detail
  {
    // internal method to split a filename into path and fullname.
    inline void splitpath(const std::string& fullname, std::string& path, std::string& name)
    {
      auto pos = fullname.rfind('/');
      if (pos != std::string::npos)
      {
        path = fullname.substr(0, pos);
        name = fullname.substr(pos+1);
      }
      else
      {
        path = ".";
        name = fullname;
      }
    }
  } // namespace detail

  /**
   * returns true if the filename exists and refers to a directory.
   */
  inline bool is_directory(const std::string& filename)
  {
#if defined(_WIN32)
    DWORD attr = GetFileAttributes(filename.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    struct stat s;
    return (stat(filename.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
#endif
  }

  /**
   * creates a new directory. returns true on success.
   */
  inline bool make_directory(const std::string& filename)
  {
#if defined(_WIN32)
    return _mkdir(filename.c_str()) == 0;
#else
    return mkdir(filename.c_str(), 0755) == 0;
#endif
  }

  /**
   * truncates a file to the given length, if the file exists and can be opened
   * for writing.
   */
  inline void truncate(const std::string& filename, size_t length)
  {
#if defined(_WIN32)
    int fd = -1;
    _sopen_s(&fd, filename.c_str(), _O_WRONLY | _O_BINARY, _SH_DENYNO, _S_IWRITE);
    if (fd != -1)
    {
      _chsize_s(fd, static_cast<__int64>(length));
      _close(fd);
    }
#else
    int error = ::truncate(filename.c_str(), static_cast<off_t>(length));
    DIY_UNUSED(error);
#endif
  }

  inline int mkstemp(std::string& filename)
  {
#if defined(_WIN32)

    std::string path, name;
    detail::splitpath(filename, path, name);
    char temppath[MAX_PATH];
    // note: GetTempFileName only uses the first 3 chars of the prefix (name)
    // specified.
    if (GetTempFileName(path.c_str(), name.c_str(), 0, temppath) == 0)
    {
      // failed! return invalid handle.
      return -1;
    }
    int handle = -1;
    // _sopen_s sets handle to -1 on error.
    _sopen_s(&handle, temppath, _O_WRONLY | _O_CREAT | _O_BINARY, _SH_DENYNO, _S_IWRITE);
    if (handle != -1)
      filename = temppath;
    return handle;

#else // defined(_WIN32)

    std::unique_ptr<char[]> s_template(new char[filename.size() + 1]);
    std::copy(filename.begin(), filename.end(), s_template.get());
    s_template[filename.size()] = 0;

    int handle = -1;
#if defined(__MACH__)
    // TODO: figure out how to open with O_SYNC
    handle = ::mkstemp(s_template.get());
#else
    handle = mkostemp(s_template.get(), O_WRONLY | O_SYNC);
#endif
    if (handle != -1)
      filename = s_template.get();
    return handle;

#endif // defined(_WIN32)
  }

  inline void close(int fd)
  {
#if defined(_WIN32)
    _close(fd);
#else
    fsync(fd);
    ::close(fd);
#endif
  }

  inline void sync(int fd)
  {
#if defined(_WIN32)
    DIY_UNUSED(fd);
#else
    fsync(fd);
#endif
  }

  inline bool remove(const std::string& filename)
  {
#if defined(_WIN32)
    return DeleteFile(filename.c_str()) != 0;
#else
    return ::remove(filename.c_str()) == 0;
#endif
  }
}
}
}

#endif
