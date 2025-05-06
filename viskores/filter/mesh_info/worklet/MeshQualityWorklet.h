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
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_filter_mesh_info_worklet_MeshQualityWorklet_h
#define viskores_filter_mesh_info_worklet_MeshQualityWorklet_h

#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/ErrorCode.h>
#include <viskores/TypeList.h>

namespace
{

/**
  * Worklet that computes mesh quality metric values for each cell in
  * the input mesh. A metric is specified per cell type in the calling filter,
  * and this metric is invoked over all cells of that cell type. An array of
  * the computed metric values (one per cell) is returned as output.
  */
template <typename Derived>
struct MeshQualityWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell metricOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);


  template <typename CellShapeType, typename PointCoordVecType, typename OutType>
  VISKORES_EXEC void operator()(CellShapeType shape,
                                const viskores::IdComponent& numPoints,
                                const PointCoordVecType& pts,
                                OutType& metricValue) const
  {
    viskores::UInt8 thisId = shape.Id;
    if (shape.Id == viskores::CELL_SHAPE_POLYGON)
    {
      if (numPoints == 3)
        thisId = viskores::CELL_SHAPE_TRIANGLE;
      else if (numPoints == 4)
        thisId = viskores::CELL_SHAPE_QUAD;
    }

    const Derived* self = reinterpret_cast<const Derived*>(this);
    viskores::ErrorCode errorCode = viskores::ErrorCode::Success;
    switch (thisId)
    {
      viskoresGenericCellShapeMacro(metricValue = self->template ComputeMetric<OutType>(
                                      numPoints, pts, CellShapeTag{}, errorCode));
      default:
        errorCode = viskores::ErrorCode::InvalidShapeId;
        metricValue = OutType(0.0);
    }

    if (errorCode != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(errorCode));
    }
  }

  VISKORES_CONT viskores::cont::UnknownArrayHandle Run(const viskores::cont::DataSet& input,
                                                       const viskores::cont::Field& field) const
  {
    if (!field.IsPointField())
    {
      throw viskores::cont::ErrorBadValue("Active field for MeshQuality must be point coordinates. "
                                          "But the active field is not a point field.");
    }

    viskores::cont::UnknownArrayHandle outArray;
    viskores::cont::Invoker invoke;

    auto resolveType = [&](const auto& concrete)
    {
      using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      viskores::cont::ArrayHandle<T> result;
      invoke(*reinterpret_cast<const Derived*>(this), input.GetCellSet(), concrete, result);
      outArray = result;
    };
    field.GetData()
      .CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldVec3,
                                            VISKORES_DEFAULT_STORAGE_LIST>(resolveType);

    return outArray;
  }
};

} // anonymous namespace

#endif //viskores_filter_mesh_info_worklet_MeshQualityWorklet_h
