// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <ctime>       // for time_t
#include <string>      // for string, operator!=, etc
#include <sys/types.h> // for off_t

namespace Ioss {

  /*! \class FileInfo
   *  \author Greg Sjaardema
   *  \brief  Return information about the specified file.
   *
   *  A very minimal class (at least it used to be) for providing
   *  information about a file.  FileInfo provides information about a
   *  file's name, path, and type (directory, symbolic link, file).  Other
   *  information could be added as needed.  It currently does not cache
   *  any information, so if it is heavily used, a caching capability
   *  should be added.  See the Qt Toolkit QFileInfo class for a richer
   *  file class.
   */

  class FileInfo
  {
  public:
    //! Empty class referring to no file.
    FileInfo();

    //! Create object referring to file with name \a filename
    //! \param my_filename name of file
    explicit FileInfo(std::string my_filename);

    //! Create object referring to file with name \a filename
    //! \param my_filename name of file
    explicit FileInfo(const char *my_filename);

    //! Copy constructor
    FileInfo(const FileInfo & /*copy_from*/);

    //! Constructor
    //! \param dirpath Directory Path
    //! \param my_filename base filename
    FileInfo(const std::string &dirpath, const std::string &my_filename);

    ~FileInfo();

    //! returns the number of processors that this file exists.
    //! 0: Exists nowhere
    //! \#proc: Exists everywhere
    //! else: exists on some proc, but not all.
    //! In the last case, a list of processors where it is missing is returned in 'where' on
    //! processor 0.
    int parallel_exists(MPI_Comm communicator, std::string &where) const;

    bool exists() const;        //!< returns True if file exists, false if nonexistent
    bool is_readable() const;   //!< Exists and is readable
    bool is_writable() const;   //!< Exists and is writable
    bool is_executable() const; //!< Exists and is executable

    bool is_file() const;    //!< Is a plain file
    bool is_dir() const;     //!< Is a directory
    bool is_symlink() const; //!< Is a symbolic link to a file or directory

    time_t modified() const; //!< Time of last data modification. See 'man stat(2)'
    time_t accessed() const; //!< Time of last access
    time_t created() const;  //!< Time of last status change. (creation, chmod, ...)

    off_t size() const; //!< File size in bytes. Only if is_file() == true

    std::string filename() const;  //!< Complete filename including path
    std::string basename() const;  //!< strip path and extension
    std::string tailname() const;  //!< basename() + extension()
    std::string extension() const; //!< file extension.
    std::string pathname() const;  //!< directory path, no filename
    std::string realpath() const;  //!< canonicalized absolute path

    void set_filename(const std::string &name);
    void set_filename(const char *name);

    bool operator==(const FileInfo &other) const { return filename_ == other.filename_; }

    bool operator!=(const FileInfo &other) const { return filename_ != other.filename_; }

    bool remove_file();

    //! This function is used to create the path to an output directory (or history, restart, etc.)
    //!  if it does not exist.  Called by all processors. Will throw exception if path does not
    //!  specify a valid directory or if the path cannot be created.
    static void create_path(const std::string &filename, MPI_Comm communicator);
    static void create_path(const std::string &filename);

  private:
    std::string filename_{};
    bool        exists_{false};   ///< this is used frequently, check on creation
    bool        readable_{false}; ///< this is used frequently, check on creation
  };
} // namespace Ioss
