//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/ADIOSDataSource.h>
#include <fides/ConduitDataSource.h>
#include <fides/DataSourceFactory.h>

namespace fides
{
namespace io
{

std::unique_ptr<DataSource> MakeDataSource(const std::string& dsType)
{
  if (dsType == "adios")
  {
    return std::make_unique<ADIOSDataSource>();
  }
#if FIDES_USE_CONDUIT
  else if (dsType == "conduit")
  {
    return std::make_unique<ConduitDataSource>();
  }
#endif
  else
  {
    throw std::runtime_error("Unrecognized data source type: " + dsType);
  }
}

#if FIDES_USE_MPI
std::unique_ptr<DataSource> MakeDataSource(const std::string& dsType, MPI_Comm comm)
{
  if (dsType == "adios")
  {
    return std::make_unique<ADIOSDataSource>(comm);
  }
#if FIDES_USE_CONDUIT
  else if (dsType == "conduit")
  {
    return std::make_unique<ConduitDataSource>(comm);
  }
#endif
  else
  {
    throw std::runtime_error("Unrecognized data source type: " + dsType);
  }
}
#endif

}
}
