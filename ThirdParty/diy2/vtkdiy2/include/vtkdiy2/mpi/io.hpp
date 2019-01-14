#ifndef DIY_MPI_IO_HPP
#define DIY_MPI_IO_HPP

#include "../constants.h"

#include <vector>
#include <string>

namespace diy
{
namespace mpi
{
namespace io
{
  typedef               MPI_Offset              offset;

  //! Wraps MPI file IO. \ingroup MPI
  class file
  {
    public:
      enum
      {
        rdonly          = MPI_MODE_RDONLY,
        rdwr            = MPI_MODE_RDWR,
        wronly          = MPI_MODE_WRONLY,
        create          = MPI_MODE_CREATE,
        exclusive       = MPI_MODE_EXCL,
        delete_on_close = MPI_MODE_DELETE_ON_CLOSE,
        unique_open     = MPI_MODE_UNIQUE_OPEN,
        sequential      = MPI_MODE_SEQUENTIAL,
        append          = MPI_MODE_APPEND
      };

    public:
      inline        file(const communicator& comm, const std::string& filename, int mode);
                    ~file()                                 { close(); }
      inline void   close();

      inline offset size() const;
      inline void   resize(offset size);

      inline void   read_at(offset o, char* buffer, size_t size);
      inline void   read_at_all(offset o, char* buffer, size_t size);
      inline void   write_at(offset o, const char* buffer, size_t size);
      inline void   write_at_all(offset o, const char* buffer, size_t size);

      template<class T>
      inline void   read_at(offset o, std::vector<T>& data);

      template<class T>
      inline void   read_at_all(offset o, std::vector<T>& data);

      template<class T>
      inline void   write_at(offset o, const std::vector<T>& data);

      template<class T>
      inline void   write_at_all(offset o, const std::vector<T>& data);

      const communicator&
                    comm() const                            { return comm_; }

      MPI_File&     handle()                                { return fh; }

    private:
      const communicator&   comm_;
      MPI_File              fh;
  };
}
}
}

diy::mpi::io::file::
file(const communicator& comm__, const std::string& filename, int mode)
: comm_(comm__)
{
#ifndef DIY_NO_MPI
  int ret = MPI_File_open(comm__, const_cast<char*>(filename.c_str()), mode, MPI_INFO_NULL, &fh);
  if (ret)
      throw std::runtime_error("DIY cannot open file: " + filename);
#else
  DIY_UNUSED(comm__);
  DIY_UNUSED(filename);
  DIY_UNUSED(mode);
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_open);
#endif
}

void
diy::mpi::io::file::
close()
{
#ifndef DIY_NO_MPI
  if (fh != MPI_FILE_NULL)
    MPI_File_close(&fh);
#endif
}

diy::mpi::io::offset
diy::mpi::io::file::
size() const
{
#ifndef DIY_NO_MPI
  offset sz;
  MPI_File_get_size(fh, &sz);
  return sz;
#else
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_get_size);
#endif
}

void
diy::mpi::io::file::
resize(diy::mpi::io::offset size_)
{
#ifndef DIY_NO_MPI
  MPI_File_set_size(fh, size_);
#else
  DIY_UNUSED(size_);
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_set_size);
#endif
}

void
diy::mpi::io::file::
read_at(offset o, char* buffer, size_t size_)
{
#ifndef DIY_NO_MPI
  status s;
  MPI_File_read_at(fh, o, buffer, static_cast<int>(size_), detail::get_mpi_datatype<char>(), &s.s);
#else
  DIY_UNUSED(o);
  DIY_UNUSED(buffer);
  DIY_UNUSED(size_);
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_read_at);
#endif
}

template<class T>
void
diy::mpi::io::file::
read_at(offset o, std::vector<T>& data)
{
  read_at(o, &data[0], data.size()*sizeof(T));
}

void
diy::mpi::io::file::
read_at_all(offset o, char* buffer, size_t size_)
{
#ifndef DIY_NO_MPI
  status s;
  MPI_File_read_at_all(fh, o, buffer, static_cast<int>(size_), detail::get_mpi_datatype<char>(), &s.s);
#else
  DIY_UNUSED(o);
  DIY_UNUSED(buffer);
  DIY_UNUSED(size_);
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_read_at_all);
#endif
}

template<class T>
void
diy::mpi::io::file::
read_at_all(offset o, std::vector<T>& data)
{
  read_at_all(o, (char*) &data[0], data.size()*sizeof(T));
}

void
diy::mpi::io::file::
write_at(offset o, const char* buffer, size_t size_)
{
#ifndef DIY_NO_MPI
  status s;
  MPI_File_write_at(fh, o, (void *)buffer, static_cast<int>(size_), detail::get_mpi_datatype<char>(), &s.s);
#else
  DIY_UNUSED(o);
  DIY_UNUSED(buffer);
  DIY_UNUSED(size_);
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_write_at);
#endif
}

template<class T>
void
diy::mpi::io::file::
write_at(offset o, const std::vector<T>& data)
{
  write_at(o, (const char*) &data[0], data.size()*sizeof(T));
}

void
diy::mpi::io::file::
write_at_all(offset o, const char* buffer, size_t size_)
{
#ifndef DIY_NO_MPI
  status s;
  MPI_File_write_at_all(fh, o, (void *)buffer, static_cast<int>(size_), detail::get_mpi_datatype<char>(), &s.s);
#else
  DIY_UNUSED(o);
  DIY_UNUSED(buffer);
  DIY_UNUSED(size_);
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_write_at_all);
#endif
}

template<class T>
void
diy::mpi::io::file::
write_at_all(offset o, const std::vector<T>& data)
{
  write_at_all(o, &data[0], data.size()*sizeof(T));
}

#endif
