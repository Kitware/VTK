#ifdef DIY_MPI_AS_LIB
#include "operations.hpp"
#endif

#include <functional>

namespace diy
{
namespace mpi
{

namespace detail
{

operation get_builtin_operation(BuiltinOperation id)
{
  operation op{};
  switch(id)
  {
    case OP_MAXIMUM:     op.handle = make_DIY_MPI_Op(MPI_MAX);  break;
    case OP_MINIMUM:     op.handle = make_DIY_MPI_Op(MPI_MIN);  break;
    case OP_PLUS:        op.handle = make_DIY_MPI_Op(MPI_SUM);  break;
    case OP_MULTIPLIES:  op.handle = make_DIY_MPI_Op(MPI_PROD); break;
    case OP_LOGICAL_AND: op.handle = make_DIY_MPI_Op(MPI_LAND); break;
    case OP_LOGICAL_OR:  op.handle = make_DIY_MPI_Op(MPI_LOR);  break;
    default: break;
  }
  return op;
}

}
}
} // diy::mpi::detail
