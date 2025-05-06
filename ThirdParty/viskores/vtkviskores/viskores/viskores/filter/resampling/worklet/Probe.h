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
#ifndef viskores_worklet_Probe_h
#define viskores_worklet_Probe_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellLocatorChooser.h>
#include <viskores/cont/Invoker.h>
#include <viskores/exec/CellInside.h>
#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/VecFromPortalPermute.h>

namespace viskores
{
namespace worklet
{

class Probe
{
  //============================================================================
public:
  class FindCellWorklet : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn points,
                                  ExecObject locator,
                                  FieldOut cellIds,
                                  FieldOut pcoords);
    using ExecutionSignature = void(_1, _2, _3, _4);

    template <typename LocatorType>
    VISKORES_EXEC void operator()(const viskores::Vec3f& point,
                                  const LocatorType& locator,
                                  viskores::Id& cellId,
                                  viskores::Vec3f& pcoords) const
    {
      locator.FindCell(point, cellId, pcoords);
    }
  };

private:
  struct RunSelectLocator
  {
    template <typename LocatorType, typename PointsType>
    void operator()(const LocatorType& locator, Probe& worklet, const PointsType& points) const
    {
      worklet.Invoke(
        FindCellWorklet{}, points, locator, worklet.CellIds, worklet.ParametricCoordinates);
    }
  };

  template <typename CellSetType, typename PointsType, typename PointsStorage>
  void RunImpl(const CellSetType& cells,
               const viskores::cont::CoordinateSystem& coords,
               const viskores::cont::ArrayHandle<PointsType, PointsStorage>& points)
  {
    this->InputCellSet = viskores::cont::UnknownCellSet(cells);

    viskores::cont::CastAndCallCellLocatorChooser(cells, coords, RunSelectLocator{}, *this, points);
  }

  //============================================================================
public:
  class ProbeUniformPoints : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint coords,
                                  WholeArrayIn points,
                                  WholeArrayInOut cellIds,
                                  WholeArrayOut parametricCoords);
    using ExecutionSignature = void(InputIndex, CellShape, _2, _3, _4, _5);
    using InputDomain = _1;

    template <typename CellShapeTag,
              typename CoordsVecType,
              typename UniformPoints,
              typename CellIdsType,
              typename ParametricCoordsType>
    VISKORES_EXEC void operator()(viskores::Id cellId,
                                  CellShapeTag cellShape,
                                  const CoordsVecType& cellPoints,
                                  const UniformPoints& points,
                                  CellIdsType& cellIds,
                                  ParametricCoordsType& pcoords) const
    {
      // Compute cell bounds
      using CoordsType = typename viskores::VecTraits<CoordsVecType>::ComponentType;
      auto numPoints = viskores::VecTraits<CoordsVecType>::GetNumberOfComponents(cellPoints);

      CoordsType cbmin = cellPoints[0], cbmax = cellPoints[0];
      for (viskores::IdComponent i = 1; i < numPoints; ++i)
      {
        cbmin = viskores::Min(cbmin, cellPoints[i]);
        cbmax = viskores::Max(cbmax, cellPoints[i]);
      }

      // Compute points inside cell bounds
      auto minp = static_cast<viskores::Id3>(
        viskores::Ceil((cbmin - points.GetOrigin()) / points.GetSpacing()));
      auto maxp = static_cast<viskores::Id3>(
        viskores::Floor((cbmax - points.GetOrigin()) / points.GetSpacing()));

      // clamp
      minp = viskores::Max(minp, viskores::Id3(0));
      maxp = viskores::Min(maxp, points.GetDimensions() - viskores::Id3(1));

      for (viskores::Id k = minp[2]; k <= maxp[2]; ++k)
      {
        for (viskores::Id j = minp[1]; j <= maxp[1]; ++j)
        {
          for (viskores::Id i = minp[0]; i <= maxp[0]; ++i)
          {
            auto pt = points.Get(viskores::Id3(i, j, k));
            CoordsType pc;
            viskores::ErrorCode status = viskores::exec::WorldCoordinatesToParametricCoordinates(
              cellPoints, pt, cellShape, pc);
            if ((status == viskores::ErrorCode::Success) &&
                viskores::exec::CellInside(pc, cellShape))
            {
              auto pointId = i + points.GetDimensions()[0] * (j + points.GetDimensions()[1] * k);
              cellIds.Set(pointId, cellId);
              pcoords.Set(pointId, pc);
            }
          }
        }
      }
    }
  };

private:
  template <typename CellSetType>
  void RunImpl(const CellSetType& cells,
               const viskores::cont::CoordinateSystem& coords,
               const viskores::cont::ArrayHandleUniformPointCoordinates::Superclass& points)
  {
    this->InputCellSet = viskores::cont::UnknownCellSet(cells);
    viskores::cont::ArrayCopy(
      viskores::cont::make_ArrayHandleConstant(viskores::Id(-1), points.GetNumberOfValues()),
      this->CellIds);
    this->ParametricCoordinates.Allocate(points.GetNumberOfValues());

    this->Invoke(
      ProbeUniformPoints{}, cells, coords, points, this->CellIds, this->ParametricCoordinates);
  }

  //============================================================================
  struct RunImplCaller
  {
    template <typename PointsArrayType, typename CellSetType>
    void operator()(const PointsArrayType& points,
                    Probe& worklet,
                    const CellSetType& cells,
                    const viskores::cont::CoordinateSystem& coords) const
    {
      worklet.RunImpl(cells, coords, points);
    }
  };

public:
  template <typename CellSetType, typename PointsArrayType>
  void Run(const CellSetType& cells,
           const viskores::cont::CoordinateSystem& coords,
           const PointsArrayType& points)
  {
    viskores::cont::CastAndCall(points, RunImplCaller(), *this, cells, coords);
  }

  //============================================================================
  template <typename T>
  class InterpolatePointField : public viskores::worklet::WorkletMapField
  {
  public:
    T InvalidValue;
    InterpolatePointField(const T& invalidValue)
      : InvalidValue(invalidValue)
    {
    }

    using ControlSignature = void(FieldIn cellIds,
                                  FieldIn parametricCoords,
                                  WholeCellSetIn<> inputCells,
                                  WholeArrayIn inputField,
                                  FieldOut result);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    template <typename ParametricCoordType, typename CellSetType, typename InputFieldPortalType>
    VISKORES_EXEC void operator()(viskores::Id cellId,
                                  const ParametricCoordType& pc,
                                  const CellSetType& cells,
                                  const InputFieldPortalType& in,
                                  typename InputFieldPortalType::ValueType& out) const
    {
      if (cellId != -1)
      {
        auto indices = cells.GetIndices(cellId);
        auto pointVals = viskores::make_VecFromPortalPermute(&indices, in);
        viskores::exec::CellInterpolate(pointVals, pc, cells.GetCellShape(cellId), out);
      }
      else
      {
        out = this->InvalidValue;
      }
    }
  };

  /// Intepolate the input point field data at the points of the geometry
  template <typename InArrayType,
            typename OutArrayType,
            typename ComponentType,
            typename InputCellSetTypeList = VISKORES_DEFAULT_CELL_SET_LIST>
  void ProcessPointField(const InArrayType& field,
                         const OutArrayType& result,
                         ComponentType invalidValue,
                         InputCellSetTypeList icsTypes = InputCellSetTypeList()) const
  {
    viskores::cont::Invoker invoke;
    invoke(InterpolatePointField<ComponentType>(invalidValue),
           this->CellIds,
           this->ParametricCoordinates,
           this->InputCellSet.ResetCellSetList(icsTypes),
           field,
           result);
  }

  viskores::cont::ArrayHandle<viskores::Id> GetCellIds() const { return this->CellIds; }

  //============================================================================
  struct HiddenPointsWorklet : public WorkletMapField
  {
    using ControlSignature = void(FieldIn cellids, FieldOut hfield);
    using ExecutionSignature = _2(_1);

    VISKORES_EXEC viskores::UInt8 operator()(viskores::Id cellId) const
    {
      return (cellId == -1) ? HIDDEN : 0;
    }
  };

  /// Get an array of flags marking the invalid points (points that do not fall inside any of
  /// the cells of the input). The flag value is the same as the HIDDEN flag in VTK and VISIT.
  ///
  viskores::cont::ArrayHandle<viskores::UInt8> GetHiddenPointsField() const
  {
    viskores::cont::ArrayHandle<viskores::UInt8> field;
    this->Invoke(HiddenPointsWorklet{}, this->CellIds, field);
    return field;
  }

  //============================================================================
  struct HiddenCellsWorklet : public WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellset, FieldInPoint cellids, FieldOutCell);
    using ExecutionSignature = _3(_2, PointCount);

    template <typename CellIdsVecType>
    VISKORES_EXEC viskores::UInt8 operator()(const CellIdsVecType& cellIds,
                                             viskores::IdComponent numPoints) const
    {
      for (viskores::IdComponent i = 0; i < numPoints; ++i)
      {
        if (cellIds[i] == -1)
        {
          return HIDDEN;
        }
      }
      return 0;
    }
  };

  /// Get an array of flags marking the invalid cells. Invalid cells are the cells with at least
  /// one invalid point. The flag value is the same as the HIDDEN flag in VTK and VISIT.
  ///
  template <typename CellSetType>
  viskores::cont::ArrayHandle<viskores::UInt8> GetHiddenCellsField(CellSetType cellset) const
  {
    viskores::cont::ArrayHandle<viskores::UInt8> field;
    this->Invoke(HiddenCellsWorklet{}, cellset, this->CellIds, field);
    return field;
  }

  //============================================================================
private:
  static constexpr viskores::UInt8 HIDDEN = 2; // from vtk

  viskores::cont::ArrayHandle<viskores::Id> CellIds;
  viskores::cont::ArrayHandle<viskores::Vec3f> ParametricCoordinates;
  viskores::cont::UnknownCellSet InputCellSet;

  viskores::cont::Invoker Invoke;
};
}
} // viskores::worklet

#endif // viskores_worklet_Probe_h
