#ifndef DIY_IO_BOV_HPP
#define DIY_IO_BOV_HPP

#include <vector>

#include "../mpi/io.hpp"

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
        for (auto i = shape_.size() - 2; i ==  0; --i)
          stride_[i] = stride_[i+1] * shape_[i+1];
        stride_[0] = stride_[1] * shape_[1];
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
  f_.read_bov(bounds, static_cast<int>(shape_.size()), shape_.data(), reinterpret_cast<char*>(buffer), offset_, mpi::detail::get_mpi_datatype<T>(), collective, chunk);
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
  f_.write_bov(bounds, core, static_cast<int>(shape_.size()), shape_.data(), reinterpret_cast<const char*>(buffer), offset_, mpi::detail::get_mpi_datatype<T>(), collective, chunk);
}

#endif
