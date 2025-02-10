#ifndef DIY_MPI_IO_HPP
#define DIY_MPI_IO_HPP

#include "config.hpp"
#include "communicator.hpp"

#include "../types.hpp"

#include <vector>
#include <string>
#include <stdexcept>

namespace diy
{
namespace mpi
{

namespace io
{
#if !defined(DIY_MPI_AS_LIB) && DIY_HAS_MPI
  using offset = MPI_Offset;
#else
  using offset = long long;
#endif

  //! Wraps MPI file IO. \ingroup MPI
  class file
  {
    public:
#ifndef DIY_MPI_AS_LIB
      static constexpr int rdonly          = MPI_MODE_RDONLY;
      static constexpr int rdwr            = MPI_MODE_RDWR;
      static constexpr int wronly          = MPI_MODE_WRONLY;
      static constexpr int create          = MPI_MODE_CREATE;
      static constexpr int exclusive       = MPI_MODE_EXCL;
      static constexpr int delete_on_close = MPI_MODE_DELETE_ON_CLOSE;
      static constexpr int unique_open     = MPI_MODE_UNIQUE_OPEN;
      static constexpr int sequential      = MPI_MODE_SEQUENTIAL;
      static constexpr int append          = MPI_MODE_APPEND;
#else
      static const int rdonly, rdwr, wronly, create, exclusive, delete_on_close, unique_open, sequential, append;
#endif

    public:
      DIY_MPI_EXPORT_FUNCTION        file(const communicator& comm, const std::string& filename, int mode);
                                     ~file()                            { close(); }
      DIY_MPI_EXPORT_FUNCTION void   close();

      DIY_MPI_EXPORT_FUNCTION offset size() const;
      DIY_MPI_EXPORT_FUNCTION void   resize(offset size);

      DIY_MPI_EXPORT_FUNCTION void   read_at(offset o, char* buffer, size_t size);
      DIY_MPI_EXPORT_FUNCTION void   read_at_all(offset o, char* buffer, size_t size);
      DIY_MPI_EXPORT_FUNCTION void   write_at(offset o, const char* buffer, size_t size);
      DIY_MPI_EXPORT_FUNCTION void   write_at_all(offset o, const char* buffer, size_t size);

      template<class T>
      inline void           read_at(offset o, std::vector<T>& data);

      template<class T>
      inline void           read_at_all(offset o, std::vector<T>& data);

      template<class T>
      inline void           write_at(offset o, const std::vector<T>& data);

      template<class T>
      inline void           write_at_all(offset o, const std::vector<T>& data);

      DIY_MPI_EXPORT_FUNCTION void   read_bov(const DiscreteBounds& bounds, int ndims, const int dims[], char* buffer, size_t offset, const datatype& dt, bool collective, int chunk);
      DIY_MPI_EXPORT_FUNCTION void   write_bov(const DiscreteBounds& bounds, const DiscreteBounds& core, int ndims, const int dims[], const char* buffer, size_t offset, const datatype& dt, bool collective, int chunk);

      const communicator&   comm() const   { return comm_; }

    private:
      communicator   comm_;
    protected: // mark protected to avoid the "unused private field" warning
      DIY_MPI_File   fh;
  };
}

template<class T>
void
diy::mpi::io::file::
read_at(offset o, std::vector<T>& data)
{
  read_at(o, &data[0], data.size()*sizeof(T));
}

template<class T>
void
diy::mpi::io::file::
read_at_all(offset o, std::vector<T>& data)
{
  read_at_all(o, (char*) &data[0], data.size()*sizeof(T));
}

template<class T>
void
diy::mpi::io::file::
write_at(offset o, const std::vector<T>& data)
{
  write_at(o, (const char*) &data[0], data.size()*sizeof(T));
}

template<class T>
void
diy::mpi::io::file::
write_at_all(offset o, const std::vector<T>& data)
{
  write_at_all(o, &data[0], data.size()*sizeof(T));
}

}
} // diy::mpi::io

#ifndef DIY_MPI_AS_LIB
#include "io.cpp"
#endif

#endif // DIY_MPI_IO_HPP
