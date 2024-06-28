// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"
#include "Ioss_FileInfo.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Utils.h"
#include <cstring>
#include <ostream>
#include <stdlib.h>
#include <string>
#include <tokenize.h>
#include <vector>

#if defined(__IOSS_WINDOWS__)
#include <direct.h>
#include <io.h>

#define access _access
#define R_OK   4 /* Test for read permission.  */
#define W_OK   2 /* Test for write permission.  */
#define X_OK   1 /* execute permission - unsupported in windows*/
#define F_OK   0 /* Test for existence.  */
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#else
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/mount.h>
#elif defined(__OpenBSD__)
#include <sys/mount.h>
#include <sys/types.h>
#else
#include <sys/statfs.h>
#endif
#endif

#ifdef SEACAS_HAVE_MPI
#include <numeric>
#endif

#include <cstdio>
#include <sys/stat.h>

namespace {
  bool internal_access(const std::string &name, int mode);
  bool do_stat(const std::string &filename, struct stat *s);
} // namespace

namespace Ioss {

  FileInfo::FileInfo() = default;

  FileInfo::FileInfo(std::string my_filename) : filename_(std::move(my_filename))
  {
    readable_ = internal_access(filename_, R_OK);
    exists_   = readable_ || internal_access(filename_, F_OK);
  }

  FileInfo::FileInfo(const char *my_filename) : filename_(std::string(my_filename))
  {
    readable_ = internal_access(filename_, R_OK);
    exists_   = readable_ || internal_access(filename_, F_OK);
  }

  FileInfo::FileInfo(const FileInfo &copy_from) = default;

  FileInfo::FileInfo(const std::string &dirpath, const std::string &my_filename)
  {

    if (!dirpath.empty()) {
      filename_ = dirpath;
      if (filename_.at(filename_.size() - 1) != '/') {
        static std::string SLASH("/");
        filename_ += SLASH;
      }
    }
    filename_ += my_filename;
    readable_ = internal_access(filename_, R_OK);
    exists_   = readable_ || internal_access(filename_, F_OK);
  }

  //: Returns TRUE if the file exists (is readable)
  bool FileInfo::exists() const { return exists_; }

  int FileInfo::parallel_exists(IOSS_MAYBE_UNUSED Ioss_MPI_Comm communicator,
                                IOSS_MAYBE_UNUSED std::string &where) const
  {
    IOSS_PAR_UNUSED(communicator);
    IOSS_PAR_UNUSED(where);
    int sum = exists_ ? 1 : 0;

#ifdef SEACAS_HAVE_MPI
    Ioss::ParallelUtils pu(communicator);
    int                 my_rank = pu.parallel_rank();
    int                 my_size = pu.parallel_size();
    if (my_size > 1) {
      // Handle the parallel case
      std::vector<int> result;
      pu.all_gather(sum, result);
      sum = std::accumulate(result.begin(), result.end(), 0);
      if (my_rank == 0 && sum < my_size) {
        std::vector<size_t> procs;
        for (int i = 0; i < my_size; i++) {
          if (result[i] == 0) {
            procs.push_back(i);
          }
        }
        where = Ioss::Utils::format_id_list(procs, "--");
      }
    }
#endif
    return sum;
  }

  //: Returns TRUE if the file is readable
  bool FileInfo::is_readable() const { return readable_; }

  //: Returns TRUE if the file is writable
  bool FileInfo::is_writable() const { return internal_access(filename_, W_OK); }

  //: Returns TRUE if the file is executable
  bool FileInfo::is_executable() const { return internal_access(filename_, X_OK); }

  //: Returns TRUE if we are pointing to a file or a symbolic link to
  //: a file.
  bool FileInfo::is_file() const
  {
    struct stat s
    {
    };
    if (do_stat(filename_, &s)) {
      return S_ISREG(s.st_mode);
    }

    return false;
  }

  //: Returns TRUE if we are pointing to a directory or a symbolic link to
  //: a directory.
  bool FileInfo::is_dir() const
  {
    struct stat s
    {
    };
    if (do_stat(filename_, &s)) {
      return S_ISDIR(s.st_mode);
    }

    return false;
  }

  //: Returns TRUE if we are pointing to a symbolic link
  bool FileInfo::is_symlink() const
  {
#if !defined(__IOSS_WINDOWS__)
    struct stat s
    {
    };
    if (lstat(filename_.c_str(), &s) == 0) {
      return S_ISLNK(s.st_mode);
    }
#endif
    return false;
  }

  //: Return TRUE if file is on an NFS filesystem...
  bool FileInfo::is_nfs() const
  {
#if !defined(__IOSS_WINDOWS__)
    auto tmp_path = pathname();
    if (tmp_path.empty()) {
      char *current_cwd = getcwd(nullptr, 0);
      tmp_path          = std::string(current_cwd);
      free(current_cwd);
    }
    char *path = ::realpath(tmp_path.c_str(), nullptr);
    if (path != nullptr) {

      struct statfs stat_fs;
      // We want to run `statfs` on the path; not the filename since it might not exist.
      if (statfs(path, &stat_fs) == -1) {
        free(path);
        std::ostringstream errmsg;
        errmsg << "ERROR: Could not run statfs on '" << filename_ << "'.\n";
        IOSS_ERROR(errmsg);
      }
      free(path);

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
      return (0 == ::strcmp("nfs", stat_fs.f_fstypename));
#else
      /* linux statfs defines that 0x6969 is NFS filesystem */
      return (stat_fs.f_type == 0x6969);
#endif
    }
#endif
    return false;
  }

  //: Time of last data modification. See 'man stat(2)'
  time_t FileInfo::modified() const
  {
    struct stat s
    {
    };
    if (do_stat(filename_, &s)) {
      return s.st_mtime;
    }

    return 0;
  }

  //: Time of last access
  time_t FileInfo::accessed() const
  {
    struct stat s
    {
    };
    if (do_stat(filename_, &s)) {
      return s.st_atime;
    }

    return 0;
  }

  //: Time of last status change. (creation, chmod, ...)
  time_t FileInfo::created() const
  {
    struct stat s
    {
    };
    if (do_stat(filename_, &s)) {
      return s.st_ctime;
    }

    return 0;
  }

  //: File size in bytes. Only if is_file() == true
  off_t FileInfo::size() const
  {
    struct stat s
    {
    };
    if (do_stat(filename_, &s)) {
      return s.st_size;
    }

    return 0;
  }

  //: Returns the filename
  std::string FileInfo::filename() const { return filename_; }

  //: Sets the filename
  void FileInfo::set_filename(const std::string &name)
  {
    filename_ = name;
    readable_ = internal_access(filename_, R_OK);
    exists_   = readable_ || internal_access(filename_, F_OK);
  }

  //: Sets the filename
  void FileInfo::set_filename(const char *name)
  {
    filename_ = std::string(name);
    readable_ = internal_access(filename_, R_OK);
    exists_   = readable_ || internal_access(filename_, F_OK);
  }

  //: Returns the filename extension or the empty string if there is
  //: no extension.  Assumes extension is all characters following the
  //: last period.
  std::string FileInfo::extension() const
  {
    size_t ind  = filename_.find_last_of('.', std::string::npos);
    size_t inds = filename_.find_last_of('/', std::string::npos);

    // Protect against './filename' returning /filename as extension
    if (ind != std::string::npos && (inds == std::string::npos || inds < ind)) {
      return filename_.substr(ind + 1, filename_.size());
    }

    return {};
  }

  std::string FileInfo::pathname() const
  {
    size_t ind = filename_.find_last_of('/', filename_.size());
    if (ind != std::string::npos) {
      return filename_.substr(0, ind);
    }

    return {};
  }

  std::string FileInfo::tailname() const
  {
    size_t ind = filename_.find_last_of('/', filename_.size());
    if (ind != std::string::npos) {
      return filename_.substr(ind + 1, filename_.size());
    }

    return filename_; // No path, just return the filename
  }

  std::string FileInfo::basename() const
  {
    std::string tail = tailname();

    // Strip off the extension
    size_t ind = tail.find_last_of('.', tail.size());
    if (ind != std::string::npos) {
      return tail.substr(0, ind);
    }

    return tail;
  }

  std::string FileInfo::realpath() const
  {
#if defined(__IOSS_WINDOWS__)
    char *path = _fullpath(nullptr, filename_.c_str(), _MAX_PATH);
#else
    char *path = ::realpath(filename_.c_str(), nullptr);
#endif
    if (path != nullptr) {
      std::string temp(path);
      free(path);
      return temp;
    }
    return filename_;
  }

  bool FileInfo::remove_file()
  {
    int success = std::remove(filename_.c_str());
    return success == 0;
  }

  void FileInfo::create_path(const std::string &filename)
  {
    bool               error_found = false;
    std::ostringstream errmsg;

    Ioss::FileInfo file      = Ioss::FileInfo(filename);
    std::string    path      = file.pathname();
    std::string    path_root = path[0] == '/' ? "/" : "";

    auto comps = tokenize(path, "/");
    for (const auto &comp : comps) {
      path_root += comp;

      struct stat st;
      if (stat(path_root.c_str(), &st) != 0) {
#if defined(__IOSS_WINDOWS__)
        if (mkdir(path_root.c_str()) != 0 && errno != EEXIST) {
#else
        const int mode = 0777; // Users umask will be applied to this.
        if (mkdir(path_root.c_str(), mode) != 0 && errno != EEXIST) {
#endif
          errmsg << "ERROR: Cannot create directory '" << path_root << "': " << std::strerror(errno)
                 << "\n";
          error_found = true;
          break;
        }
      }
      else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        errmsg << "ERROR: Path '" << path_root << "' is not a directory.\n";
        error_found = true;
        break;
      }
      path_root += "/";
    }

    if (error_found) {
      IOSS_ERROR(errmsg);
    }
  }

  void FileInfo::create_path(const std::string              &filename,
                             IOSS_MAYBE_UNUSED Ioss_MPI_Comm communicator)
  {
    IOSS_PAR_UNUSED(communicator);
#ifdef SEACAS_HAVE_MPI
    int                error_found = 0;
    std::ostringstream errmsg;

    Ioss::ParallelUtils util(communicator);

    if (util.parallel_rank() == 0) {
      try {
        create_path(filename);
      }
      catch (const std::exception &x) {
        errmsg << x.what();
        error_found = 1;
      }
    }
    else {
      errmsg << "ERROR: Could not create path '" << filename << "'.\n";
    }

    util.broadcast(error_found);
    if (error_found) {
      IOSS_ERROR(errmsg);
    }
#else
    create_path(filename);
#endif
  }

} // namespace Ioss

namespace {
  bool internal_access(const std::string &name, int mode)
  {
    if (name.empty()) {
      return false;
    }
    if (::access(name.c_str(), mode) != 0) {
      return false;
    }
    return true;
  }

  bool do_stat(const std::string &filename, struct stat *s)
  {
    return (stat(filename.c_str(), s) == 0);
  }
} // namespace
