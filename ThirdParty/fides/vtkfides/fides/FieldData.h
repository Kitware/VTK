//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_FieldData_H_
#define fides_datamodel_FieldData_H_

#include <fides/Deprecated.h>

#include <viskores/cont/UnknownArrayHandle.h>

#include <vector>

#include "fides_export.h"

namespace fides
{
namespace datamodel
{

/// \brief Class to store data that does not have an Association of points or cells.
///
/// Data is stored in UnknownArrayHandles, with one UnknownArrayHandle per data block.
class FIDES_DEPRECATED(1.1, "FieldData is no longer used. All data is stored in Viskores DataSet.")
  FIDES_EXPORT FieldData
{
public:
  FieldData() = default;

  FieldData(const std::string& name, const std::vector<viskores::cont::UnknownArrayHandle>&& data);

  /// Returns the name of this field
  std::string GetName() const;

  /// Get a reference to the data. Each element of the vector stores one block.
  const std::vector<viskores::cont::UnknownArrayHandle>& GetData() const;

  /// Get a reference to the data. Each element of the vector stores one block.
  std::vector<viskores::cont::UnknownArrayHandle>& GetData();

private:
  std::string Name;
  std::vector<viskores::cont::UnknownArrayHandle> Data;
};

}
}

#endif
