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

#include <vtkm/cont/VariantArrayHandle.h>

#include <vector>

#include "fides_export.h"

namespace fides
{
namespace datamodel
{

/// \brief Class to store data that does not have an Association of points or cells.
///
/// Data is stored in VariantArrayHandles, with one VariantArrayHandle per data block.
class FIDES_EXPORT FieldData
{
public:
  FieldData() = default;

  FieldData(const std::string& name, const std::vector<vtkm::cont::VariantArrayHandle>&& data);

  /// Returns the name of this field
  std::string GetName() const;

  /// Get a reference to the data. Each element of the vector stores one block.
  const std::vector<vtkm::cont::VariantArrayHandle>& GetData() const;

  /// Get a reference to the data. Each element of the vector stores one block.
  std::vector<vtkm::cont::VariantArrayHandle>& GetData();

private:
  std::string Name;
  std::vector<vtkm::cont::VariantArrayHandle> Data;
};

}
}

#endif
