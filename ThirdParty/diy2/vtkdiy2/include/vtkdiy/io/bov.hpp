#ifndef DIY_IO_BOV_HPP
#define DIY_IO_BOV_HPP

#include <vector>
#include <algorithm>
#include <numeric>

#include "../types.hpp"
#include "../mpi.hpp"

namespace diy
{
namespace io
{
  // Reads and writes subsets of a block of values into specified block bounds
  class BOV
  {
    public:
      typedef       std::vector<int>                                    Shape;
    public:
                    BOV(mpi::io::file&    f):
                      f_(f), offset_(0)                                 {}

      template<class S>
                    BOV(mpi::io::file&    f,
                        const S&          shape  = S(),
                        mpi::io::offset   offset = 0):
                      f_(f), offset_(offset)                            { set_shape(shape); }

      void          set_offset(mpi::io::offset offset)                  { offset_ = offset; }

      template<class S>
      void          set_shape(const S& shape)
      {
        shape_.clear();
        stride_.clear();
        for (unsigned i = 0; i < shape.size(); ++i)
        {
            shape_.push_back(shape[i]);
            stride_.push_back(1);
        }
        for (int i = shape_.size() - 2; i >=  0; --i)
          stride_[i] = stride_[i+1] * shape_[i+1];
      }

      const Shape&  shape() const                                       { return shape_; }

      template<class T>
      void          read(const DiscreteBounds& bounds, T* buffer, bool collective = false, int chunk = 1) const;

      template<class T>
      void          write(const DiscreteBounds& bounds, const T* buffer, bool collective = false, int chunk = 1);

      template<class T>
      void          write(const DiscreteBounds& bounds, const T* buffer, const DiscreteBounds& core, bool collective = false, int chunk = 1);

    protected:
      mpi::io::file&        file()                                        { return f_; }

    private:
      mpi::io::file&        f_;
      Shape                 shape_;
      std::vector<size_t>   stride_;
      size_t                offset_;
  };
}
}

template<class T>
void
diy::io::BOV::
read(const DiscreteBounds& bounds, T* buffer, bool collective, int chunk) const
{
  int dim   = shape_.size();
  int total = 1;
  std::vector<int> subsizes;
  for (int i = 0; i < dim; ++i)
  {
    subsizes.push_back(bounds.max[i] - bounds.min[i] + 1);
    total *= subsizes.back();
  }

  MPI_Datatype T_type;
  if (chunk == 1)
    T_type = mpi::detail::get_mpi_datatype<T>();
  else
  {
    // create an MPI struct of size chunk to read the data in those chunks
    // (this allows to work around MPI-IO weirdness where crucial quantities
    // are ints, which are too narrow of a type)
    int             array_of_blocklengths[]  = { chunk };
    MPI_Aint        array_of_displacements[] = { 0 };
    MPI_Datatype    array_of_types[]         = { mpi::detail::get_mpi_datatype<T>() };
    MPI_Type_create_struct(1, array_of_blocklengths, array_of_displacements, array_of_types, &T_type);
    MPI_Type_commit(&T_type);
  }

  MPI_Datatype fileblk;
  MPI_Type_create_subarray(dim, (int*) &shape_[0], &subsizes[0], (int*) &bounds.min[0], MPI_ORDER_C, T_type, &fileblk);
  MPI_Type_commit(&fileblk);

  MPI_File_set_view(f_.handle(), offset_, T_type, fileblk, (char*)"native", MPI_INFO_NULL);

  mpi::status s;
  if (!collective)
      MPI_File_read(f_.handle(), buffer, total, T_type, &s.s);
  else
      MPI_File_read_all(f_.handle(), buffer, total, T_type, &s.s);

  if (chunk != 1)
    MPI_Type_free(&T_type);
  MPI_Type_free(&fileblk);
}

template<class T>
void
diy::io::BOV::
write(const DiscreteBounds& bounds, const T* buffer, bool collective, int chunk)
{
    write(bounds, buffer, bounds, collective, chunk);
}

template<class T>
void
diy::io::BOV::
write(const DiscreteBounds& bounds, const T* buffer, const DiscreteBounds& core, bool collective, int chunk)
{
  int dim   = shape_.size();
  std::vector<int> subsizes;
  std::vector<int> buffer_shape, buffer_start;
  for (int i = 0; i < dim; ++i)
  {
    buffer_shape.push_back(bounds.max[i] - bounds.min[i] + 1);
    buffer_start.push_back(core.min[i] - bounds.min[i]);
    subsizes.push_back(core.max[i] - core.min[i] + 1);
  }

  MPI_Datatype T_type;
  if (chunk == 1)
    T_type = mpi::detail::get_mpi_datatype<T>();
  else
  {
    // assume T is a binary block and create an MPI struct of appropriate size
    int             array_of_blocklengths[]  = { chunk };
    MPI_Aint        array_of_displacements[] = { 0 };
    MPI_Datatype    array_of_types[]         = { mpi::detail::get_mpi_datatype<T>() };
    MPI_Type_create_struct(1, array_of_blocklengths, array_of_displacements, array_of_types, &T_type);
    MPI_Type_commit(&T_type);
  }

  MPI_Datatype fileblk, subbuffer;
  MPI_Type_create_subarray(dim, (int*) &shape_[0],       &subsizes[0], (int*) &bounds.min[0],   MPI_ORDER_C, T_type, &fileblk);
  MPI_Type_create_subarray(dim, (int*) &buffer_shape[0], &subsizes[0], (int*) &buffer_start[0], MPI_ORDER_C, T_type, &subbuffer);
  MPI_Type_commit(&fileblk);
  MPI_Type_commit(&subbuffer);

  MPI_File_set_view(f_.handle(), offset_, T_type, fileblk, (char*)"native", MPI_INFO_NULL);

  mpi::status s;
  if (!collective)
    MPI_File_write(f_.handle(), (void*)buffer, 1, subbuffer, &s.s);
  else
    MPI_File_write_all(f_.handle(), (void*)buffer, 1, subbuffer, &s.s);

  if (chunk != 1)
    MPI_Type_free(&T_type);
  MPI_Type_free(&fileblk);
  MPI_Type_free(&subbuffer);
}

#endif
