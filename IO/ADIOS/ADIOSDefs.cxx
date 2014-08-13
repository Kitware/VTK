#include "ADIOSDefs.h"

namespace ADIOS
{

const std::string&  ToString(TransportMethod method)
{
  static const std::string valueMap[] = {
    "NULL", "POSIX", "MPI", "MPI_LUSTRE", "MPI_AGGREGATE", "VAR_MERGE",
    "Dataspaces", "DIMES", "Flexpath", "PHDF5", "NetCDF4" };
  return valueMap[method];
}

const std::string& ToString(Transform xfm)
{
  static const std::string valueMap[] = { "", "zlib", "bzlib2", "szip" };
  return valueMap[xfm];
}

}
