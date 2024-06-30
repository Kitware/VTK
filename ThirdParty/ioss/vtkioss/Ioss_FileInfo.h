// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
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

  class IOSS_EXPORT FileInfo
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

    //! returns the number of processors that this file exists.
    //! 0: Exists nowhere
    //! \#proc: Exists everywhere
    //! else: exists on some proc, but not all.
    //! In the last case, a list of processors where it is missing is returned in 'where' on
    //! processor 0.
    IOSS_NODISCARD int parallel_exists(Ioss_MPI_Comm communicator, std::string &where) const;

    IOSS_NODISCARD bool exists() const;      //!< returns True if file exists, false if nonexistent
    IOSS_NODISCARD bool is_readable() const; //!< Exists and is readable
    IOSS_NODISCARD bool is_writable() const; //!< Exists and is writable
    IOSS_NODISCARD bool is_executable() const; //!< Exists and is executable

    IOSS_NODISCARD bool is_file() const;    //!< Is a plain file
    IOSS_NODISCARD bool is_dir() const;     //!< Is a directory
    IOSS_NODISCARD bool is_symlink() const; //!< Is a symbolic link to a file or directory
    IOSS_NODISCARD bool is_nfs() const;     //!< Is on an NFS filesystem

    IOSS_NODISCARD time_t modified() const; //!< Time of last data modification. See 'man stat(2)'
    IOSS_NODISCARD time_t accessed() const; //!< Time of last access
    IOSS_NODISCARD time_t created() const;  //!< Time of last status change. (creation, chmod, ...)

    IOSS_NODISCARD off_t size() const; //!< File size in bytes. Only if is_file() == true

    IOSS_NODISCARD std::string filename() const;  //!< Complete filename including path
    IOSS_NODISCARD std::string basename() const;  //!< strip path and extension
    IOSS_NODISCARD std::string tailname() const;  //!< basename() + extension()
    IOSS_NODISCARD std::string extension() const; //!< file extension.
    IOSS_NODISCARD std::string pathname() const;  //!< directory path, no filename
    IOSS_NODISCARD std::string realpath() const;  //!< canonicalized absolute path

    void set_filename(const std::string &name);
    void set_filename(const char *name);

    IOSS_NODISCARD bool operator==(const FileInfo &other) const
    {
      return filename_ == other.filename_;
    }

    IOSS_NODISCARD bool operator!=(const FileInfo &other) const
    {
      return filename_ != other.filename_;
    }

    bool remove_file();

    //! This function is used to create the path to an output directory (or history, restart, etc.)
    //!  if it does not exist.  Called by all processors. Will throw exception if path does not
    //!  specify a valid directory or if the path cannot be created.
    static void create_path(const std::string              &filename,
                            IOSS_MAYBE_UNUSED Ioss_MPI_Comm communicator);
    static void create_path(const std::string &filename);

  private:
    std::string filename_{};
    bool        exists_{false};   ///< this is used frequently, check on creation
    bool        readable_{false}; ///< this is used frequently, check on creation
  };
} // namespace Ioss
