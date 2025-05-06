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

#include <viskores/filter/geometry_refinement/ConvertToPointCloud.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/CellSetSingleType.h>

#include <viskores/CellShape.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

viskores::cont::DataSet ConvertToPointCloud::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::Id numPoints = input.GetNumberOfPoints();

  // A connectivity array for a point cloud is easy. All the cells are a vertex with exactly
  // one point. So, it can be represented a simple index array (i.e., 0, 1, 2, 3, ...).
  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex{ numPoints }, connectivity);

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(numPoints, viskores::CELL_SHAPE_VERTEX, 1, connectivity);

  auto fieldMapper = [&](viskores::cont::DataSet& outData, viskores::cont::Field& field)
  {
    if (field.IsCellField())
    {
      // Cell fields are dropped.
      return;
    }
    else if (this->AssociateFieldsWithCells && field.IsPointField() &&
             !input.HasCoordinateSystem(field.GetName()))
    {
      // The user asked to convert point fields to cell fields. (They are interchangable in
      // point clouds.)
      outData.AddCellField(field.GetName(), field.GetData());
    }
    else
    {
      outData.AddField(field);
    }
  };
  return this->CreateResult(input, cellSet, fieldMapper);
}

}
}
} // namespace viskores::filter::geometry_refinement
