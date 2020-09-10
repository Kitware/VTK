//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_predefined_SupportedDataModels_H_
#define fides_predefined_SupportedDataModels_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

namespace fides
{
namespace predefined
{

/// Enum for the data models that are currently supported
enum class DataModelTypes
{
  UNIFORM,
  RECTILINEAR,
  UNSTRUCTURED,
  UNSTRUCTURED_SINGLE,
  XGC,
  UNSUPPORTED
};

/// Converts string identifying data model to the DataModelTypes enum
DataModelTypes ConvertDataModelToEnum(std::string str);

/// Checks if the specified data model can be generated
bool DataModelSupported(const std::string& dataModel);

}
}

#endif
