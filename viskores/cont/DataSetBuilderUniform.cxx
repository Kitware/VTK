//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/DataSetBuilderUniform.h>

namespace viskores
{
namespace cont
{

VISKORES_CONT
DataSetBuilderUniform::DataSetBuilderUniform() {}

VISKORES_CONT
viskores::cont::DataSet DataSetBuilderUniform::Create(const viskores::Id& dimension,
                                                      const std::string& coordNm)
{
  return CreateDataSet(viskores::Id3(dimension, 1, 1), VecType(0), VecType(1), coordNm);
}

VISKORES_CONT
viskores::cont::DataSet DataSetBuilderUniform::Create(const viskores::Id2& dimensions,
                                                      const std::string& coordNm)
{
  return CreateDataSet(
    viskores::Id3(dimensions[0], dimensions[1], 1), VecType(0), VecType(1), coordNm);
}

VISKORES_CONT
viskores::cont::DataSet DataSetBuilderUniform::Create(const viskores::Id3& dimensions,
                                                      const std::string& coordNm)
{
  return CreateDataSet(
    viskores::Id3(dimensions[0], dimensions[1], dimensions[2]), VecType(0), VecType(1), coordNm);
}

VISKORES_CONT
viskores::cont::DataSet DataSetBuilderUniform::CreateDataSet(const viskores::Id3& dimensions,
                                                             const viskores::Vec3f& origin,
                                                             const viskores::Vec3f& spacing,
                                                             const std::string& coordNm)
{
  viskores::Id dims[3] = { 1, 1, 1 };
  int ndims = 0;
  for (int i = 0; i < 3; ++i)
  {
    if (dimensions[i] > 1)
    {
      if (spacing[i] <= 0.0f)
      {
        throw viskores::cont::ErrorBadValue("spacing must be > 0.0");
      }
      dims[ndims++] = dimensions[i];
    }
  }

  viskores::cont::DataSet dataSet;
  viskores::cont::ArrayHandleUniformPointCoordinates coords(dimensions, origin, spacing);
  viskores::cont::CoordinateSystem cs(coordNm, coords);
  dataSet.AddCoordinateSystem(cs);

  if (ndims == 1)
  {
    viskores::cont::CellSetStructured<1> cellSet;
    cellSet.SetPointDimensions(dims[0]);
    dataSet.SetCellSet(cellSet);
  }
  else if (ndims == 2)
  {
    viskores::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(viskores::Id2(dims[0], dims[1]));
    dataSet.SetCellSet(cellSet);
  }
  else if (ndims == 3)
  {
    viskores::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(viskores::Id3(dims[0], dims[1], dims[2]));
    dataSet.SetCellSet(cellSet);
  }
  else
  {
    throw viskores::cont::ErrorBadValue("Invalid cell set dimension");
  }

  return dataSet;
}
}
} // end namespace viskores::cont
