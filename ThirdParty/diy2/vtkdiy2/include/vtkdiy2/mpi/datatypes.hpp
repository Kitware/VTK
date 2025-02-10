#ifndef DIY_MPI_DATATYPES_HPP
#define DIY_MPI_DATATYPES_HPP

#include "config.hpp"

#include <vector>
#include <array>
#include <cstddef>

namespace diy
{
namespace mpi
{

struct datatype
{
  datatype() = default;
  datatype(const DIY_MPI_Datatype& dt) : handle(dt) {}

#ifndef DIY_MPI_AS_LIB // only available in header-only mode
  datatype(const MPI_Datatype& dt) : handle(dt) {}
  operator MPI_Datatype() { return handle; }
#endif

  DIY_MPI_Datatype handle;
};

namespace detail
{
  struct true_type  {};
  struct false_type {};

  /* is_mpi_datatype */
  template<class T>
  struct is_mpi_datatype        { typedef false_type    type; };

  template<class T> datatype  get_mpi_datatype();

  #define DIY_MPI_DATATYPE_DEFAULT(cpp_type)                                                      \
  template<> DIY_MPI_EXPORT_FUNCTION datatype get_mpi_datatype<cpp_type>();                       \
  template<>  struct is_mpi_datatype< cpp_type >                { typedef true_type type; };      \
  template<>  struct is_mpi_datatype< std::vector<cpp_type> >   { typedef true_type type; };      \
  template<size_t N>                                                                              \
              struct is_mpi_datatype< std::array<cpp_type, N> > { typedef true_type type; };

  DIY_MPI_DATATYPE_DEFAULT(char)
  DIY_MPI_DATATYPE_DEFAULT(unsigned char)
  DIY_MPI_DATATYPE_DEFAULT(bool)
  DIY_MPI_DATATYPE_DEFAULT(int)
  DIY_MPI_DATATYPE_DEFAULT(unsigned)
  DIY_MPI_DATATYPE_DEFAULT(long)
  DIY_MPI_DATATYPE_DEFAULT(unsigned long)
  DIY_MPI_DATATYPE_DEFAULT(long long)
  DIY_MPI_DATATYPE_DEFAULT(unsigned long long)
  DIY_MPI_DATATYPE_DEFAULT(float)
  DIY_MPI_DATATYPE_DEFAULT(double)

  #undef DIY_MPI_DATATYPE_DEFAULT

  /* mpi_datatype: helper routines, specialized for std::vector<...>, std::array<...> */
  template<class T>
  struct mpi_datatype
  {
    static diy::mpi::datatype   datatype()              { return get_mpi_datatype<T>(); }
    static const void*          address(const T& x)     { return &x; }
    static void*                address(T& x)           { return &x; }
    static int                  count(const T&)         { return 1; }
  };

  template<class U>
  struct mpi_datatype< std::vector<U> >
  {
    typedef     std::vector<U>      VecU;

    static diy::mpi::datatype   datatype()              { return mpi_datatype<U>::datatype(); }
    static const void*          address(const VecU& x)  { return x.data(); }
    static void*                address(VecU& x)        { return x.data(); }
    static int                  count(const VecU& x)    { return x.empty() ? 0 : (static_cast<int>(x.size()) * mpi_datatype<U>::count(x[0])); }
  };

  template<class U, size_t D>
  struct mpi_datatype< std::array<U,D> >
  {
    typedef     std::array<U,D> ArrayU;

    static diy::mpi::datatype   datatype()                  { return mpi_datatype<U>::datatype(); }
    static const void*          address(const ArrayU& x)    { return x.data(); }
    static void*                address(ArrayU& x)          { return x.data(); }
    static int                  count(const ArrayU& x)      { return x.empty() ? 0 : (static_cast<int>(x.size()) * mpi_datatype<U>::count(x[0])); }
  };
} // detail

template<class U>
static datatype datatype_of(const U&)
{
    return detail::mpi_datatype<U>::datatype();
}

template<class U>
static void* address(const U& x)
{
    return const_cast<void*>(detail::mpi_datatype<U>::address(x));
}

template<class U>
static void* address(U& x)
{
    return detail::mpi_datatype<U>::address(x);
}

template<class U>
static int count(const U& x)
{
    return detail::mpi_datatype<U>::count(x);
}

} // mpi
} // diy

#ifndef DIY_MPI_AS_LIB
#include "datatypes.cpp"
#endif

#endif // DIY_MPI_DATATYPES_HPP
