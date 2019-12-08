#ifndef DIY_MPI_DATATYPES_HPP
#define DIY_MPI_DATATYPES_HPP

#include <vector>
#include <array>

namespace diy
{
namespace mpi
{
namespace detail
{
  template<class T> MPI_Datatype  get_mpi_datatype();

  struct true_type  {};
  struct false_type {};

  /* is_mpi_datatype */
  template<class T>
  struct is_mpi_datatype        { typedef false_type    type; };

#define DIY_MPI_DATATYPE_MAP(cpp_type, mpi_type) \
  template<>  inline MPI_Datatype  get_mpi_datatype<cpp_type>() { return mpi_type; }  \
  template<>  struct is_mpi_datatype<cpp_type>                  { typedef true_type type; };    \
  template<>  struct is_mpi_datatype< std::vector<cpp_type> >   { typedef true_type type; };    \
  template<size_t D>  \
              struct is_mpi_datatype< std::array<cpp_type,D> >  { typedef true_type type; };

  DIY_MPI_DATATYPE_MAP(char,                  MPI_BYTE);
  DIY_MPI_DATATYPE_MAP(unsigned char,         MPI_BYTE);
  DIY_MPI_DATATYPE_MAP(bool,                  MPI_BYTE);
  DIY_MPI_DATATYPE_MAP(int,                   MPI_INT);
  DIY_MPI_DATATYPE_MAP(unsigned,              MPI_UNSIGNED);
  DIY_MPI_DATATYPE_MAP(long,                  MPI_LONG);
  DIY_MPI_DATATYPE_MAP(unsigned long,         MPI_UNSIGNED_LONG);
  DIY_MPI_DATATYPE_MAP(long long,             MPI_LONG_LONG_INT);
  DIY_MPI_DATATYPE_MAP(unsigned long long,    MPI_UNSIGNED_LONG_LONG);
  DIY_MPI_DATATYPE_MAP(float,                 MPI_FLOAT);
  DIY_MPI_DATATYPE_MAP(double,                MPI_DOUBLE);

  /* mpi_datatype: helper routines, specialized for std::vector<...>, std::array<...> */
  template<class T>
  struct mpi_datatype
  {
    static MPI_Datatype         datatype()              { return get_mpi_datatype<T>(); }
    static const void*          address(const T& x)     { return &x; }
    static void*                address(T& x)           { return &x; }
    static int                  count(const T&)         { return 1; }
  };

  template<class U>
  struct mpi_datatype< std::vector<U> >
  {
    typedef     std::vector<U>      VecU;

    static MPI_Datatype         datatype()              { return mpi_datatype<U>::datatype(); }
    static const void*          address(const VecU& x)  { return x.data(); }
    static void*                address(VecU& x)        { return x.data(); }
    static int                  count(const VecU& x)    { return x.empty() ? 0 : (static_cast<int>(x.size()) * mpi_datatype<U>::count(x[0])); }
  };

  template<class U, size_t D>
  struct mpi_datatype< std::array<U,D> >
  {
    typedef     std::array<U,D> ArrayU;

    static MPI_Datatype         datatype()                  { return mpi_datatype<U>::datatype(); }
    static const void*          address(const ArrayU& x)    { return x.data(); }
    static void*                address(ArrayU& x)          { return x.data(); }
    static int                  count(const ArrayU& x)      { return x.empty() ? 0 : (static_cast<int>(x.size()) * mpi_datatype<U>::count(x[0])); }
  };
} // detail

template<class U>
static MPI_Datatype datatype(const U&)
{
    using Datatype = detail::mpi_datatype<U>;
    return Datatype::datatype();
}

template<class U>
static void* address(const U& x)
{
    using Datatype = detail::mpi_datatype<U>;
    return const_cast<void*>(Datatype::address(x));
}

template<class U>
static void* address(U& x)
{
    using Datatype = detail::mpi_datatype<U>;
    return Datatype::address(x);
}

template<class U>
static int count(const U& x)
{
    using Datatype = detail::mpi_datatype<U>;
    return Datatype::count(x);
}



} // mpi
} // diy

#endif
