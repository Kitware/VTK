#ifndef DIY_MPI_OPERATIONS_HPP
#define DIY_MPI_OPERATIONS_HPP

#include "config.hpp"

#include <algorithm> // for std::min/max
#include <functional>

namespace diy
{
namespace mpi
{
  //! \addtogroup MPI
  //!@{
  struct operation
  {
    operation() = default;
    operation(const DIY_MPI_Op& op) : handle(op) {}

#ifndef DIY_MPI_AS_LIB // only available in header-only mode
    operation(const MPI_Op& op) : handle(op) {}
    operator MPI_Op() { return handle; }
#endif

    DIY_MPI_Op handle;
  };

  template<class U>
  struct maximum { const U& operator()(const U& x, const U& y) const { return (std::max)(x,y); } };
  template<class U>
  struct minimum { const U& operator()(const U& x, const U& y) const { return (std::min)(x,y); } };
  //!@}

namespace detail
{
  enum BuiltinOperation {
    OP_MAXIMUM = 0,
    OP_MINIMUM,
    OP_PLUS,
    OP_MULTIPLIES,
    OP_LOGICAL_AND,
    OP_LOGICAL_OR
  };

  DIY_MPI_EXPORT_FUNCTION operation get_builtin_operation(BuiltinOperation id);

  template<class T> struct mpi_op;

  template<class U> struct mpi_op< maximum<U> >          { static operation get() { return get_builtin_operation(OP_MAXIMUM); } };
  template<class U> struct mpi_op< minimum<U> >          { static operation get() { return get_builtin_operation(OP_MINIMUM); } };
  template<class U> struct mpi_op< std::plus<U> >        { static operation get() { return get_builtin_operation(OP_PLUS); } };
  template<class U> struct mpi_op< std::multiplies<U> >  { static operation get() { return get_builtin_operation(OP_MULTIPLIES); } };
  template<class U> struct mpi_op< std::logical_and<U> > { static operation get() { return get_builtin_operation(OP_LOGICAL_AND); } };
  template<class U> struct mpi_op< std::logical_or<U> >  { static operation get() { return get_builtin_operation(OP_LOGICAL_OR); } };
}

}
} // diy::mpi

#ifndef DIY_MPI_AS_LIB
#include "operations.cpp"
#endif

#endif // DIY_MPI_OPERATIONS_HPP
