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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//=========================================================================

#include <viskores/filter/mesh_info/MeshQualityArea.h>
#include <viskores/filter/mesh_info/MeshQualityRelativeSizeSquared.h>
#include <viskores/filter/mesh_info/MeshQualityVolume.h>

#include <viskores/filter/mesh_info/worklet/MeshQualityWorklet.h>
#include <viskores/filter/mesh_info/worklet/cellmetrics/CellRelativeSizeSquaredMetric.h>

#include <viskores/CellTraits.h>

namespace
{

struct RelativeSizeSquaredWorklet : MeshQualityWorklet<RelativeSizeSquaredWorklet>
{
  viskores::Float64 AverageArea;
  viskores::Float64 AverageVolume;

  VISKORES_CONT RelativeSizeSquaredWorklet(viskores::Float64 averageArea,
                                           viskores::Float64 averageVolume)
    : AverageArea(averageArea)
    , AverageVolume(averageVolume)
  {
  }

  VISKORES_EXEC viskores::Float64 GetAverageSize(viskores::CellTopologicalDimensionsTag<2>) const
  {
    return this->AverageArea;
  }
  VISKORES_EXEC viskores::Float64 GetAverageSize(viskores::CellTopologicalDimensionsTag<3>) const
  {
    return this->AverageVolume;
  }
  template <viskores::IdComponent Dimension>
  VISKORES_EXEC viskores::Float64 GetAverageSize(
    viskores::CellTopologicalDimensionsTag<Dimension>) const
  {
    return 1;
  }

  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VISKORES_EXEC OutType ComputeMetric(const viskores::IdComponent& numPts,
                                      const PointCoordVecType& pts,
                                      CellShapeType shape,
                                      viskores::ErrorCode& ec) const
  {
    using DimensionTag = typename viskores::CellTraits<CellShapeType>::TopologicalDimensionsTag;
    return viskores::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
      numPts, pts, static_cast<OutType>(this->GetAverageSize(DimensionTag{})), shape, ec);
  }
};

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace mesh_info
{

MeshQualityRelativeSizeSquared::MeshQualityRelativeSizeSquared()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName("relativeSizeSquared");
}

viskores::cont::DataSet MeshQualityRelativeSizeSquared::DoExecute(
  const viskores::cont::DataSet& input)
{
  RelativeSizeSquaredWorklet worklet(
    viskores::filter::mesh_info::MeshQualityArea{}.ComputeAverageArea(input),
    viskores::filter::mesh_info::MeshQualityVolume{}.ComputeAverageVolume(input));
  viskores::cont::UnknownArrayHandle outArray =
    worklet.Run(input, this->GetFieldFromDataSet(input));

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), outArray);
}

} // namespace mesh_info
} // namespace filter
} // namespace viskores
