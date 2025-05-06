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

#ifndef viskores_filter_flow_worklet_GridEvaluators_h
#define viskores_filter_flow_worklet_GridEvaluators_h

#include <viskores/CellClassification.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellLocatorGeneral.h>
#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>

#include <viskores/filter/flow/worklet/CellInterpolationHelper.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/GridEvaluatorStatus.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename FieldType>
class ExecutionGridEvaluator
{
  using GhostCellArrayType = viskores::cont::ArrayHandle<viskores::UInt8>;
  using ExecFieldType = typename FieldType::ExecutionType;

public:
  VISKORES_CONT
  ExecutionGridEvaluator() = default;

  VISKORES_CONT
  ExecutionGridEvaluator(const viskores::cont::CellLocatorGeneral& locator,
                         const viskores::cont::CellInterpolationHelper interpolationHelper,
                         const viskores::Bounds& bounds,
                         const FieldType& field,
                         const GhostCellArrayType& ghostCells,
                         viskores::cont::DeviceAdapterId device,
                         viskores::cont::Token& token)
    : Bounds(bounds)
    , Field(field.PrepareForExecution(device, token))
    , GhostCells(ghostCells.PrepareForInput(device, token))
    , HaveGhostCells(ghostCells.GetNumberOfValues() > 0)
    , InterpolationHelper(interpolationHelper.PrepareForExecution(device, token))
    , Locator(locator.PrepareForExecution(device, token))
  {
  }

  template <typename Point>
  VISKORES_EXEC bool IsWithinSpatialBoundary(const Point& point) const
  {
    viskores::Id cellId = -1;
    Point parametric;

    this->Locator.FindCell(point, cellId, parametric);

    if (cellId == -1)
      return false;
    else
      return !this->InGhostCell(cellId);
  }

  VISKORES_EXEC
  bool IsWithinTemporalBoundary(const viskores::FloatDefault& viskoresNotUsed(time)) const
  {
    return true;
  }

  VISKORES_EXEC
  viskores::Bounds GetSpatialBoundary() const { return this->Bounds; }

  VISKORES_EXEC_CONT
  viskores::FloatDefault GetTemporalBoundary(viskores::Id direction) const
  {
    // Return the time of the newest time slice
    return direction > 0 ? viskores::Infinity<viskores::FloatDefault>()
                         : viskores::NegativeInfinity<viskores::FloatDefault>();
  }

  template <typename Point>
  VISKORES_EXEC GridEvaluatorStatus HelpEvaluate(const Point& point,
                                                 const viskores::FloatDefault& time,
                                                 viskores::VecVariable<Point, 2>& out) const
  {
    viskores::Id cellId = -1;
    Point parametric;
    GridEvaluatorStatus status;

    status.SetOk();
    if (!this->IsWithinTemporalBoundary(time))
    {
      status.SetFail();
      status.SetTemporalBounds();
    }

    this->Locator.FindCell(point, cellId, parametric);
    if (cellId == -1)
    {
      status.SetFail();
      status.SetSpatialBounds();
    }
    else if (this->InGhostCell(cellId))
    {
      status.SetFail();
      status.SetInGhostCell();
      status.SetSpatialBounds();
    }

    //If initial checks ok, then do the evaluation.
    if (status.CheckOk())
    {
      viskores::UInt8 cellShape;
      viskores::IdComponent nVerts;
      viskores::VecVariable<viskores::Id, 8> ptIndices;
      viskores::VecVariable<viskores::Vec3f, 8> fieldValues;

      if (this->Field.GetAssociation() == viskores::cont::Field::Association::Points)
      {
        this->InterpolationHelper.GetCellInfo(cellId, cellShape, nVerts, ptIndices);
        this->Field.GetValue(ptIndices, nVerts, parametric, cellShape, out);
      }
      else if (this->Field.GetAssociation() == viskores::cont::Field::Association::Cells)
      {
        this->Field.GetValue(cellId, out);
      }

      status.SetOk();
    }
    return status;
  }

  template <typename Point>
  VISKORES_EXEC GridEvaluatorStatus
  DeligateEvaluateToField(const Point& point,
                          const viskores::FloatDefault& time,
                          viskores::VecVariable<Point, 2>& out) const
  {
    GridEvaluatorStatus status;
    status.SetOk();
    // TODO: Allow for getting status from deligated work from Field
    if (!this->Field.GetValue(point, time, out, this->Locator, this->InterpolationHelper))
    {
      status.SetFail();
      status.SetSpatialBounds();
    }
    return status;
  }

  template <typename Point>
  VISKORES_EXEC GridEvaluatorStatus Evaluate(const Point& point,
                                             const viskores::FloatDefault& time,
                                             viskores::VecVariable<Point, 2>& out) const
  {
    if (!ExecFieldType::DelegateToField::value)
    {
      return this->HelpEvaluate(point, time, out);
    }
    else
    {
      return this->DeligateEvaluateToField(point, time, out);
    }
  }

private:
  VISKORES_EXEC bool InGhostCell(const viskores::Id& cellId) const
  {
    if (this->HaveGhostCells && cellId != -1)
      return GhostCells.Get(cellId) == viskores::CellClassification::Ghost;

    return false;
  }

  using GhostCellPortal = typename viskores::cont::ArrayHandle<viskores::UInt8>::ReadPortalType;

  viskores::Bounds Bounds;
  ExecFieldType Field;
  GhostCellPortal GhostCells;
  bool HaveGhostCells;
  viskores::exec::CellInterpolationHelper InterpolationHelper;
  typename viskores::cont::CellLocatorGeneral::ExecObjType Locator;
};

template <typename FieldType>
class GridEvaluator : public viskores::cont::ExecutionObjectBase
{
public:
  using UniformType = viskores::cont::ArrayHandleUniformPointCoordinates;
  using AxisHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using RectilinearType =
    viskores::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using Structured2DType = viskores::cont::CellSetStructured<2>;
  using Structured3DType = viskores::cont::CellSetStructured<3>;
  using GhostCellArrayType = viskores::cont::ArrayHandle<viskores::UInt8>;

  VISKORES_CONT
  GridEvaluator() = default;

  VISKORES_CONT
  GridEvaluator(const viskores::cont::DataSet& dataSet, const FieldType& field)
    : Bounds(dataSet.GetCoordinateSystem().GetBounds())
    , Field(field)
    , GhostCellArray()
  {
    this->InitializeLocator(dataSet.GetCoordinateSystem(), dataSet.GetCellSet());

    if (dataSet.HasGhostCellField())
    {
      auto arr = dataSet.GetGhostCellField().GetData();
      viskores::cont::ArrayCopyShallowIfPossible(arr, this->GhostCellArray);
    }
  }

  VISKORES_CONT
  GridEvaluator(const viskores::cont::CoordinateSystem& coordinates,
                const viskores::cont::UnknownCellSet& cellset,
                const FieldType& field)
    : Bounds(coordinates.GetBounds())
    , Field(field)
    , GhostCellArray()
  {
    this->InitializeLocator(coordinates, cellset);
  }

  VISKORES_CONT ExecutionGridEvaluator<FieldType> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return ExecutionGridEvaluator<FieldType>(this->Locator,
                                             this->InterpolationHelper,
                                             this->Bounds,
                                             this->Field,
                                             this->GhostCellArray,
                                             device,
                                             token);
  }

private:
  VISKORES_CONT void InitializeLocator(const viskores::cont::CoordinateSystem& coordinates,
                                       const viskores::cont::UnknownCellSet& cellset)
  {
    this->Locator.SetCoordinates(coordinates);
    this->Locator.SetCellSet(cellset);
    this->Locator.Update();
    this->InterpolationHelper = viskores::cont::CellInterpolationHelper(cellset);
  }

  viskores::Bounds Bounds;
  FieldType Field;
  GhostCellArrayType GhostCellArray;
  viskores::cont::CellInterpolationHelper InterpolationHelper;
  viskores::cont::CellLocatorGeneral Locator;
};

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_GridEvaluators_h
