//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/predefined/SupportedDataModels.h>

namespace fides
{
namespace predefined
{

DataModelTypes ConvertDataModelToEnum(std::string str)
{
  const std::map<std::string, DataModelTypes> enumStrings {
    {"uniform", DataModelTypes::UNIFORM},
    {"rectilinear", DataModelTypes::RECTILINEAR},
    {"unstructured", DataModelTypes::UNSTRUCTURED},
    {"unstructured_single", DataModelTypes::UNSTRUCTURED_SINGLE},
    {"xgc", DataModelTypes::XGC}
  };
  auto it = enumStrings.find(str);
  return it == enumStrings.end() ? DataModelTypes::UNSUPPORTED : it->second;
}

bool DataModelSupported(const std::string& dataModel)
{
  return ConvertDataModelToEnum(dataModel) != DataModelTypes::UNSUPPORTED;
}

}
}
