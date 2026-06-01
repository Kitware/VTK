//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataSourceFactory_H_
#define fides_datamodel_DataSourceFactory_H_

#include "fides/DataSource.h"

#include <memory>
#include <string>

namespace fides
{
namespace io
{

/// @brief Factory method to create a DataSource subtype
/// @param dsType String indicating what subtype of DataSource to create
/// @return unique ptr to newly created object
std::unique_ptr<fides::io::DataSource> MakeDataSource(const std::string& dsType);

#if FIDES_USE_MPI
/// @brief Factory method to create a DataSource subtype, given a specific communicator
/// @param dsType String indicating what subtype of DataSource to create
/// @param comm MPI communicator to pass to subtype constructor
/// @return unique ptr to newly created object
std::unique_ptr<fides::io::DataSource> MakeDataSource(const std::string& dsType, MPI_Comm comm);
#endif

}
}

#endif
