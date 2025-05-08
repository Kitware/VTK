//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef viskores_worklet_MIR_h
#define viskores_worklet_MIR_h

#include <viskores/CellShape.h>
#include <viskores/Types.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/Timer.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/DispatcherReduceByKey.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/filter/contour/worklet/clip/ClipTables.h>
#include <viskores/filter/contour/worklet/mir/MIRTables.h>

namespace viskores
{
namespace worklet
{

struct MIRStats
{
  viskores::Id NumberOfCells = 0;
  viskores::Id NumberOfIndices = 0;
  viskores::Id NumberOfEdgeIndices = 0;

  //VISKORES New point stats
  viskores::Id NumberOfInCellPoints = 0;
  viskores::Id NumberOfInCellIndices = 0;
  viskores::Id NumberOfInCellInterpPoints = 0;
  viskores::Id NumberOfInCellEdgeIndices = 0;

  struct SumOp
  {
    VISKORES_EXEC_CONT
    MIRStats operator()(const MIRStats& stat1, const MIRStats& stat2) const
    {
      MIRStats sum = stat1;
      sum.NumberOfCells += stat2.NumberOfCells;
      sum.NumberOfIndices += stat2.NumberOfIndices;
      sum.NumberOfEdgeIndices += stat2.NumberOfEdgeIndices;
      sum.NumberOfInCellPoints += stat2.NumberOfInCellPoints;
      sum.NumberOfInCellIndices += stat2.NumberOfInCellIndices;
      sum.NumberOfInCellInterpPoints += stat2.NumberOfInCellInterpPoints;
      sum.NumberOfInCellEdgeIndices += stat2.NumberOfInCellEdgeIndices;
      return sum;
    }
  };
};
struct EdgeInterpolation
{
  viskores::Id Vertex1 = -1;
  viskores::Id Vertex2 = -1;
  viskores::Float64 Weight = 0;

  struct LessThanOp
  {
    VISKORES_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return (v1.Vertex1 < v2.Vertex1) || (v1.Vertex1 == v2.Vertex1 && v1.Vertex2 < v2.Vertex2);
    }
  };

  struct EqualToOp
  {
    VISKORES_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return v1.Vertex1 == v2.Vertex1 && v1.Vertex2 == v2.Vertex2;
    }
  };
};
namespace MIRinternal
{
template <typename T>
VISKORES_EXEC_CONT T Scale(const T& val, viskores::Float64 scale)
{
  return static_cast<T>(scale * static_cast<viskores::Float64>(val));
}

template <typename T, viskores::IdComponent NumComponents>
VISKORES_EXEC_CONT viskores::Vec<T, NumComponents> Scale(const viskores::Vec<T, NumComponents>& val,
                                                         viskores::Float64 scale)
{
  return val * scale;
}
}

class ExecutionConnectivityExplicit
{
private:
  using UInt8Portal = typename viskores::cont::ArrayHandle<viskores::UInt8>::WritePortalType;
  using IdComponentPortal =
    typename viskores::cont::ArrayHandle<viskores::IdComponent>::WritePortalType;
  using IdPortal = typename viskores::cont::ArrayHandle<viskores::Id>::WritePortalType;

public:
  VISKORES_CONT
  ExecutionConnectivityExplicit() = default;

  VISKORES_CONT
  ExecutionConnectivityExplicit(viskores::cont::ArrayHandle<viskores::UInt8> shapes,
                                viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices,
                                viskores::cont::ArrayHandle<viskores::Id> connectivity,
                                viskores::cont::ArrayHandle<viskores::Id> offsets,
                                MIRStats stats,
                                viskores::cont::DeviceAdapterId device,
                                viskores::cont::Token& token)
    : Shapes(shapes.PrepareForOutput(stats.NumberOfCells, device, token))
    , NumberOfIndices(numberOfIndices.PrepareForOutput(stats.NumberOfCells, device, token))
    , Connectivity(connectivity.PrepareForOutput(stats.NumberOfIndices, device, token))
    , Offsets(offsets.PrepareForOutput(stats.NumberOfCells, device, token))
  {
  }

  VISKORES_EXEC
  void SetCellShape(viskores::Id cellIndex, viskores::UInt8 shape)
  {
    this->Shapes.Set(cellIndex, shape);
  }

  VISKORES_EXEC
  void SetNumberOfIndices(viskores::Id cellIndex, viskores::IdComponent numIndices)
  {
    this->NumberOfIndices.Set(cellIndex, numIndices);
  }

  VISKORES_EXEC
  void SetIndexOffset(viskores::Id cellIndex, viskores::Id indexOffset)
  {
    this->Offsets.Set(cellIndex, indexOffset);
  }

  VISKORES_EXEC
  void SetConnectivity(viskores::Id connectivityIndex, viskores::Id pointIndex)
  {
    this->Connectivity.Set(connectivityIndex, pointIndex);
  }

private:
  UInt8Portal Shapes;
  IdComponentPortal NumberOfIndices;
  IdPortal Connectivity;
  IdPortal Offsets;
};

class ConnectivityExplicit : viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT
  ConnectivityExplicit() = default;

  VISKORES_CONT
  ConnectivityExplicit(const viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                       const viskores::cont::ArrayHandle<viskores::IdComponent>& numberOfIndices,
                       const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
                       const viskores::cont::ArrayHandle<viskores::Id>& offsets,
                       const MIRStats& stats)
    : Shapes(shapes)
    , NumberOfIndices(numberOfIndices)
    , Connectivity(connectivity)
    , Offsets(offsets)
    , Stats(stats)
  {
  }

  VISKORES_CONT ExecutionConnectivityExplicit
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    ExecutionConnectivityExplicit execConnectivity(this->Shapes,
                                                   this->NumberOfIndices,
                                                   this->Connectivity,
                                                   this->Offsets,
                                                   this->Stats,
                                                   device,
                                                   token);
    return execConnectivity;
  }

private:
  viskores::cont::ArrayHandle<viskores::UInt8> Shapes;
  viskores::cont::ArrayHandle<viskores::IdComponent> NumberOfIndices;
  viskores::cont::ArrayHandle<viskores::Id> Connectivity;
  viskores::cont::ArrayHandle<viskores::Id> Offsets;
  viskores::worklet::MIRStats Stats;
};

class ComputeStats : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  viskores::Id targ;
  VISKORES_CONT ComputeStats(viskores::Id target)
    : targ(target)
  {
  }
  VISKORES_CONT ComputeStats() = default;
  using ControlSignature = void(CellSetIn,
                                WholeArrayIn curVals,
                                WholeArrayIn prevVals,
                                FieldInCell offsets,
                                ExecObject mirTables,
                                FieldInCell parentObj,
                                FieldInCell prevCol,
                                FieldOutCell stats,
                                FieldOutCell caseID);
  using ExecutionSignature = void(CellShape, PointCount, _3, _2, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  template <typename CellShapeTag,
            typename ScalarFieldVec,
            typename ScalarFieldVec1,
            typename DeviceAdapter,
            typename ScalarPos,
            typename ParentObj,
            typename PreCol>
  VISKORES_EXEC void operator()(
    const CellShapeTag shape,
    const viskores::IdComponent pointCount,
    const ScalarFieldVec& prevVals,
    const ScalarFieldVec1& newVals,
    const ScalarPos& valPositionStart,
    const viskores::worklet::MIRCases::MIRTables::MIRDevicePortal<DeviceAdapter>& MIRData,
    const ParentObj&,
    const PreCol& prevCol,
    MIRStats& MIRStat,
    viskores::Id& MIRDataIndex) const
  {
    (void)shape;
    viskores::Id caseId = 0;
    if (prevCol == viskores::Id(-1))
    {
      // In case of this being the first material for the cell, automatically set it to the furthest case (that is, same shape, color 1)
      for (viskores::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        caseId++;
        if (iter > 0)
        {
          caseId *= 2;
        }
      }
    }
    else
    {
      for (viskores::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        if (static_cast<viskores::Float64>(prevVals.Get(valPositionStart + iter)) <=
            static_cast<viskores::Float64>(newVals.Get(valPositionStart + iter)))
        {
          caseId++;
        }
        if (iter > 0)
        {
          caseId *= 2;
        }
      }
    }
    // Reinitialize all struct values to 0, experienced weird memory bug otherwise, might be an issue with development environment
    MIRStat.NumberOfCells = 0;
    MIRStat.NumberOfEdgeIndices = 0;
    MIRStat.NumberOfInCellEdgeIndices = 0;
    MIRStat.NumberOfInCellIndices = 0;
    MIRStat.NumberOfInCellInterpPoints = 0;
    MIRStat.NumberOfInCellPoints = 0;
    MIRStat.NumberOfIndices = 0;
    viskores::Id index = MIRData.GetCaseIndex(shape.Id, caseId, pointCount);
    MIRDataIndex = viskores::Id(caseId);
    viskores::Id numberOfCells = MIRData.GetNumberOfShapes(shape.Id, caseId, pointCount);
    if (numberOfCells == -1)
    {
      this->RaiseError("Getting a size index of a polygon with more points than 8 or less points "
                       "than 3. Bad case.");
      return;
    }
    MIRStat.NumberOfCells = numberOfCells;

    for (viskores::IdComponent shapes = 0; shapes < numberOfCells; shapes++)
    {
      viskores::UInt8 cellType = MIRData.ValueAt(index++);
      // SH_PNT is a specification that a center point is to be used
      // Note: It is only possible to support 1 midpoint with the current code format
      if (cellType == MIRCases::SH_PNT)
      {
        MIRStat.NumberOfCells = numberOfCells - 1;
        viskores::UInt8 numberOfIndices = MIRData.ValueAt(index + 2);
        index += 3;
        MIRStat.NumberOfInCellPoints = 1;
        MIRStat.NumberOfInCellInterpPoints = numberOfIndices;
        for (viskores::IdComponent points = 0; points < numberOfIndices; points++)
        {
          viskores::Id elem = MIRData.ValueAt(index);
          // If the midpoint needs to reference an edge point, record it.
          MIRStat.NumberOfInCellEdgeIndices += (elem >= MIRCases::EA) ? 1 : 0;
          index++;
        }
      }
      else
      {
        viskores::Id numberOfIndices = MIRData.GetNumberOfIndices(cellType);
        index++;
        MIRStat.NumberOfIndices += numberOfIndices;
        for (viskores::IdComponent points = 0; points < numberOfIndices; points++, index++)
        {
          viskores::IdComponent element = MIRData.ValueAt(index);
          if (element >= MIRCases::EA && element <= MIRCases::EL)
          {
            MIRStat.NumberOfEdgeIndices++;
          }
          else if (element == MIRCases::N0)
          {
            // N0 stands for the midpoint. Technically it could be N0->N3, but with the current
            // setup, only N0 is supported/present in the MIRCases tables.
            MIRStat.NumberOfInCellIndices++;
          }
        }
      }
    }
  }
};
class MIRParentObject : public viskores::cont::ExecutionAndControlObjectBase
{
public:
  VISKORES_CONT MIRParentObject() = default;
  VISKORES_CONT MIRParentObject(viskores::Id numCells,
                                viskores::cont::ArrayHandle<viskores::Id> celllook,
                                viskores::cont::ArrayHandle<viskores::Id> cellCol,
                                viskores::cont::ArrayHandle<viskores::Id> newCellCol,
                                viskores::cont::ArrayHandle<viskores::Id> newcellLook)
    : newCellColors(newCellCol)
    , newCellLookback(newcellLook)
    , numberOfInd(numCells)
    , cellLookback(celllook)
    , cellColors(cellCol){};

  class MIRParentPortal
  {
  public:
    VISKORES_EXEC void SetNewCellLookback(viskores::Id index, viskores::Id originalIndex)
    {
      this->NewCellLookback.Set(index, originalIndex);
    }
    VISKORES_EXEC void SetNewCellColor(viskores::Id index, viskores::Id col)
    {
      this->NewCellColors.Set(index, col);
    }
    VISKORES_EXEC viskores::Id GetParentCellIndex(viskores::Id index)
    {
      return this->CellLookback.Get(index);
    }
    VISKORES_EXEC viskores::Id GetParentCellColor(viskores::Id index)
    {
      return this->CellColors.Get(index);
    }

  private:
    typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType CellLookback;
    typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType CellColors;
    typename viskores::cont::ArrayHandle<viskores::Id>::WritePortalType NewCellColors;
    typename viskores::cont::ArrayHandle<viskores::Id>::WritePortalType NewCellLookback;
    friend class MIRParentObject;
  };

  VISKORES_CONT MIRParentPortal PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                    viskores::cont::Token& token)
  {
    MIRParentPortal dev;
    dev.CellLookback = this->cellLookback.PrepareForInput(device, token);
    dev.CellColors = this->cellColors.PrepareForInput(device, token);
    dev.NewCellColors = this->newCellColors.PrepareForOutput(this->numberOfInd, device, token);
    dev.NewCellLookback = this->newCellLookback.PrepareForOutput(this->numberOfInd, device, token);
    return dev;
  }
  viskores::cont::ArrayHandle<viskores::Id> newCellColors;
  viskores::cont::ArrayHandle<viskores::Id> newCellLookback;

private:
  viskores::Id numberOfInd;
  viskores::cont::ArrayHandle<viskores::Id> cellLookback;
  viskores::cont::ArrayHandle<viskores::Id> cellColors;
};
class GenerateCellSet : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  VISKORES_EXEC_CONT
  GenerateCellSet(viskores::Id tar)
    : target(tar)
  {
  }

  using ControlSignature = void(CellSetIn,
                                WholeArrayIn prevVals,
                                WholeArrayIn newVals,
                                FieldInCell vf_pos,
                                FieldInCell mirTableIndices,
                                FieldInCell mirStats,
                                ExecObject mirTables,
                                ExecObject connectivityObject,
                                WholeArrayOut edgePointReverseConnectivity,
                                WholeArrayOut edgePointInterpolation,
                                WholeArrayOut inCellReverseConnectivity,
                                WholeArrayOut inCellEdgeReverseConnectivity,
                                WholeArrayOut inCellEdgeInterpolation,
                                WholeArrayOut inCellInterpolationKeys,
                                WholeArrayOut inCellInterpolationInfo,
                                ExecObject cellLookbackObj,
                                WholeArrayOut simpleLookback);

  using ExecutionSignature = void(CellShape,
                                  InputIndex,
                                  PointCount,
                                  PointIndices,
                                  _2,
                                  _3,
                                  _4,
                                  _5,
                                  _6,
                                  _7,
                                  _8,
                                  _9,
                                  _10,
                                  _11,
                                  _12,
                                  _13,
                                  _14,
                                  _15,
                                  _16,
                                  _17); // 20! NO MORE ROOM!

  template <typename CellShapeTag,
            typename PointVecType,
            typename ScalarVecType1,
            typename ScalarVecType2,
            typename ConnectivityObject,
            typename IdArrayType,
            typename EdgeInterpolationPortalType,
            typename DeviceAdapter,
            typename ScalarPos,
            typename CellLookbackArr>
  VISKORES_EXEC void operator()(
    const CellShapeTag shape,
    const viskores::Id workIndex,
    const viskores::IdComponent pointcount,
    const PointVecType points,
    const ScalarVecType1& curScalars,  // Previous VF
    const ScalarVecType2& newScalars,  // New VF
    const ScalarPos& valPositionStart, // Offsets into the ^ arrays for indexing
    const viskores::Id& clipDataIndex,
    const MIRStats mirStats,
    const worklet::MIRCases::MIRTables::MIRDevicePortal<DeviceAdapter>& MIRData,
    ConnectivityObject& connectivityObject,
    IdArrayType& edgePointReverseConnectivity,
    EdgeInterpolationPortalType& edgePointInterpolation,
    IdArrayType& inCellReverseConnectivity,
    IdArrayType& inCellEdgeReverseConnectivity,
    EdgeInterpolationPortalType& inCellEdgeInterpolation,
    IdArrayType& inCellInterpolationKeys,
    IdArrayType& inCellInterpolationInfo,
    worklet::MIRParentObject::MIRParentPortal& parentObj,
    CellLookbackArr& cellLookbackArray) const
  {

    (void)shape;
    viskores::Id clipIndex = MIRData.GetCaseIndex(shape.Id, clipDataIndex, pointcount);

    // Start index for the cells of this case.
    viskores::Id cellIndex = mirStats.NumberOfCells;
    // Start index to store connevtivity of this case.
    viskores::Id connectivityIndex = mirStats.NumberOfIndices;
    // Start indices for reverse mapping into connectivity for this case.
    viskores::Id edgeIndex = mirStats.NumberOfEdgeIndices;
    viskores::Id inCellIndex = mirStats.NumberOfInCellIndices;
    viskores::Id inCellPoints = mirStats.NumberOfInCellPoints;
    // Start Indices to keep track of interpolation points for new cell.
    viskores::Id inCellInterpPointIndex = mirStats.NumberOfInCellInterpPoints;
    viskores::Id inCellEdgeInterpIndex = mirStats.NumberOfInCellEdgeIndices;

    // Iterate over the shapes for the current cell and begin to fill connectivity.
    viskores::Id numberOfCells = MIRData.GetNumberOfShapes(shape.Id, clipDataIndex, pointcount);

    for (viskores::Id cell = 0; cell < numberOfCells; ++cell)
    {
      viskores::UInt8 cellShape = MIRData.ValueAt(clipIndex++);
      if (cellShape == MIRCases::SH_PNT)
      {
        clipIndex += 2;
        viskores::IdComponent numberOfPoints = MIRData.ValueAt(clipIndex);
        clipIndex++;
        // Case for a new cell point

        // 1. Output the input cell id for which we need to generate new point.
        // 2. Output number of points used for interpolation.
        // 3. If vertex
        //    - Add vertex to connectivity interpolation information.
        // 4. If edge
        //    - Add edge interpolation information for new points.
        //    - Reverse connectivity map for new points.
        // Make an array which has all the elements that need to be used
        // for interpolation.
        for (viskores::IdComponent point = 0; point < numberOfPoints;
             point++, inCellInterpPointIndex++, clipIndex++)
        {
          viskores::IdComponent entry =
            static_cast<viskores::IdComponent>(MIRData.ValueAt(clipIndex));
          inCellInterpolationKeys.Set(inCellInterpPointIndex, workIndex);
          if (entry <= MIRCases::P7)
          {
            inCellInterpolationInfo.Set(inCellInterpPointIndex, points[entry]);
          }
          else
          {
            internal::ClipTablesBase::EdgeVec edge =
              MIRData.GetEdge(shape.Id, entry - MIRCases::EA, pointcount);
            if (edge[0] == 255 || edge[1] == 255)
            {
              this->RaiseError("Edge vertices are assigned incorrect values.");
              return;
            }

            EdgeInterpolation ei;
            ei.Vertex1 = points[edge[0]];
            ei.Vertex2 = points[edge[1]];
            // For consistency purposes keep the points ordered.
            if (ei.Vertex1 > ei.Vertex2)
            {
              this->swap(ei.Vertex1, ei.Vertex2);
              this->swap(edge[0], edge[1]);
            }
            // need to swap the weight of the point to be A-C / ((D-C) - (B-A)),
            // where A and C are edge0 mats 1 and 2, and B and D are edge1 mats 1 and 2.
            ei.Weight = viskores::Float64(1) +
              ((static_cast<viskores::Float64>(curScalars.Get(valPositionStart + edge[0]) -
                                               newScalars.Get(valPositionStart + edge[0]))) /
               static_cast<viskores::Float64>(curScalars.Get(valPositionStart + edge[1]) -
                                              curScalars.Get(valPositionStart + edge[0]) +
                                              newScalars.Get(valPositionStart + edge[0]) -
                                              newScalars.Get(valPositionStart + edge[1])));

            inCellEdgeReverseConnectivity.Set(inCellEdgeInterpIndex, inCellInterpPointIndex);
            inCellEdgeInterpolation.Set(inCellEdgeInterpIndex, ei);
            inCellEdgeInterpIndex++;
          }
        }
      }
      else
      {
        viskores::IdComponent numberOfPoints =
          static_cast<viskores::IdComponent>(MIRData.GetNumberOfIndices(cellShape));
        viskores::IdComponent colorQ =
          static_cast<viskores::IdComponent>(MIRData.ValueAt(clipIndex++));
        viskores::Id color = colorQ == viskores::IdComponent(MIRCases::COLOR0)
          ? parentObj.GetParentCellColor(workIndex)
          : target;
        parentObj.SetNewCellColor(cellIndex, color);
        parentObj.SetNewCellLookback(cellIndex, parentObj.GetParentCellIndex(workIndex));
        connectivityObject.SetCellShape(cellIndex, cellShape);
        connectivityObject.SetNumberOfIndices(cellIndex, numberOfPoints);
        connectivityObject.SetIndexOffset(cellIndex, connectivityIndex);

        for (viskores::IdComponent point = 0; point < numberOfPoints; point++, clipIndex++)
        {
          viskores::IdComponent entry =
            static_cast<viskores::IdComponent>(MIRData.ValueAt(clipIndex));
          if (entry == MIRCases::N0) // case of cell point interpolation
          {
            // Add index of the corresponding cell point.
            inCellReverseConnectivity.Set(inCellIndex++, connectivityIndex);
            connectivityObject.SetConnectivity(connectivityIndex, inCellPoints);
            connectivityIndex++;
          }
          else if (entry <= MIRCases::P7) // existing vertex
          {
            connectivityObject.SetConnectivity(connectivityIndex, points[entry]);
            connectivityIndex++;
          }
          else // case of a new edge point
          {
            internal::ClipTablesBase::EdgeVec edge =
              MIRData.GetEdge(shape.Id, entry - MIRCases::EA, pointcount);
            if (edge[0] == 255 || edge[1] == 255)
            {
              this->RaiseError("Edge vertices are assigned incorrect values.");
              return;
            }
            EdgeInterpolation ei;
            ei.Vertex1 = points[edge[0]];
            ei.Vertex2 = points[edge[1]];
            // For consistency purposes keep the points ordered.
            if (ei.Vertex1 > ei.Vertex2)
            {
              this->swap(ei.Vertex1, ei.Vertex2);
              this->swap(edge[0], edge[1]);
            }

            ei.Weight = viskores::Float64(1) +
              ((static_cast<viskores::Float64>(curScalars.Get(valPositionStart + edge[0]) -
                                               newScalars.Get(valPositionStart + edge[0]))) /
               static_cast<viskores::Float64>(curScalars.Get(valPositionStart + edge[1]) -
                                              curScalars.Get(valPositionStart + edge[0]) +
                                              newScalars.Get(valPositionStart + edge[0]) -
                                              newScalars.Get(valPositionStart + edge[1])));
            //Add to set of new edge points
            //Add reverse connectivity;
            edgePointReverseConnectivity.Set(edgeIndex, connectivityIndex++);
            edgePointInterpolation.Set(edgeIndex, ei);
            edgeIndex++;
          }
        }
        // Set cell matID...
        cellLookbackArray.Set(cellIndex, workIndex);
        ++cellIndex;
      }
    }
  }

  template <typename T>
  VISKORES_EXEC void swap(T& v1, T& v2) const
  {
    T temp = v1;
    v1 = v2;
    v2 = temp;
  }

private:
  viskores::Id target;
};
class ScatterEdgeConnectivity : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  ScatterEdgeConnectivity(viskores::Id edgePointOffset)
    : EdgePointOffset(edgePointOffset)
  {
  }

  using ControlSignature = void(FieldIn sourceValue,
                                FieldIn destinationIndices,
                                WholeArrayOut destinationData);

  using ExecutionSignature = void(_1, _2, _3);

  using InputDomain = _1;

  template <typename ConnectivityDataType>
  VISKORES_EXEC void operator()(const viskores::Id sourceValue,
                                const viskores::Id destinationIndex,
                                ConnectivityDataType& destinationData) const
  {
    destinationData.Set(destinationIndex, (sourceValue + EdgePointOffset));
  }

private:
  viskores::Id EdgePointOffset;
};
class ScatterInCellConnectivity : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  ScatterInCellConnectivity(viskores::Id inCellPointOffset)
    : InCellPointOffset(inCellPointOffset)
  {
  }

  using ControlSignature = void(FieldIn destinationIndices, WholeArrayOut destinationData);

  using ExecutionSignature = void(_1, _2);

  using InputDomain = _1;

  template <typename ConnectivityDataType>
  VISKORES_EXEC void operator()(const viskores::Id destinationIndex,
                                ConnectivityDataType& destinationData) const
  {
    auto sourceValue = destinationData.Get(destinationIndex);
    destinationData.Set(destinationIndex, (sourceValue + InCellPointOffset));
  }

private:
  viskores::Id InCellPointOffset;
};
class MIR
{
public:
  MIR()
    : MIRTablesInstance()
    , EdgePointsInterpolation()
    , InCellInterpolationKeys()
    , InCellInterpolationInfo()
    , CellMapOutputToInput()
    , EdgePointsOffset()
    , InCellPointsOffset()
  {
  }
  template <typename VFList1, typename VFList2, typename CellSet, typename VFLocs, typename IDList>
  viskores::cont::CellSetExplicit<> Run(const CellSet& cellSet,
                                        const VFList1& prevValues,
                                        const VFList2& curValues,
                                        const VFLocs& offsets,
                                        const IDList& prevIDs,
                                        const viskores::Id& newID,
                                        const IDList& prevLookback,
                                        IDList& newIDs,
                                        IDList& newLookback)
  {
    // First compute the stats for the MIR algorithm & build the offsets
    //{
    ComputeStats statWorklet(newID);
    viskores::worklet::DispatcherMapTopology<ComputeStats> statsDispatch(statWorklet);

    // Output variables
    viskores::cont::ArrayHandle<MIRStats> mirStats;
    viskores::cont::ArrayHandle<viskores::Id> mirInd;

    statsDispatch.Invoke(cellSet,
                         curValues,
                         prevValues,
                         offsets,
                         this->MIRTablesInstance,
                         prevLookback,
                         prevIDs,
                         mirStats,
                         mirInd);
    // Sum all stats to form an offset array (for indexing in the MIR algorithm)
    MIRStats zero;
    viskores::cont::ArrayHandle<MIRStats> cellSetStats;
    MIRStats total =
      viskores::cont::Algorithm::ScanExclusive(mirStats, cellSetStats, MIRStats::SumOp(), zero);
    mirStats.ReleaseResources();
    //}
    // Secondly, build the sets.
    //{
    // CellSetExplicit sets
    viskores::cont::ArrayHandle<viskores::UInt8> shapes;
    viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices;
    viskores::cont::ArrayHandle<viskores::Id> connectivity;
    viskores::cont::ArrayHandle<viskores::Id> offset;
    ConnectivityExplicit connectivityObject(shapes, numberOfIndices, connectivity, offset, total);
    // Connectivity related sets
    viskores::cont::ArrayHandle<viskores::Id> edgePointReverseConnectivity;
    edgePointReverseConnectivity.Allocate(total.NumberOfEdgeIndices);
    viskores::cont::ArrayHandle<EdgeInterpolation> edgeInterpolation;
    edgeInterpolation.Allocate(total.NumberOfEdgeIndices);
    viskores::cont::ArrayHandle<viskores::Id> cellPointReverseConnectivity;
    cellPointReverseConnectivity.Allocate(total.NumberOfInCellIndices);
    viskores::cont::ArrayHandle<viskores::Id> cellPointEdgeReverseConnectivity;
    cellPointEdgeReverseConnectivity.Allocate(total.NumberOfInCellEdgeIndices);
    viskores::cont::ArrayHandle<EdgeInterpolation> cellPointEdgeInterpolation;
    cellPointEdgeInterpolation.Allocate(total.NumberOfInCellEdgeIndices);
    this->InCellInterpolationKeys.Allocate(total.NumberOfInCellInterpPoints);
    this->InCellInterpolationInfo.Allocate(total.NumberOfInCellInterpPoints);
    this->CellMapOutputToInput.Allocate(total.NumberOfCells);


    //}
    // Thirdly, call the MIR generator
    //{
    GenerateCellSet cellSetWorklet(newID);
    viskores::worklet::DispatcherMapTopology<GenerateCellSet> cellSetDispatcher(cellSetWorklet);
    // Output arrays storing information about cell lookbacks and cell material IDs
    viskores::cont::ArrayHandle<viskores::Id> nextID, nextLookback;
    nextID.Allocate(total.NumberOfCells);
    nextLookback.Allocate(total.NumberOfCells);
    MIRParentObject po(total.NumberOfCells, prevLookback, prevIDs, nextID, nextLookback);


    // Perform the MIR step
    cellSetDispatcher.Invoke(cellSet,
                             prevValues,
                             curValues,
                             offsets,
                             mirInd,
                             cellSetStats,
                             this->MIRTablesInstance,
                             connectivityObject,
                             edgePointReverseConnectivity,
                             edgeInterpolation,
                             cellPointReverseConnectivity,
                             cellPointEdgeReverseConnectivity,
                             cellPointEdgeInterpolation,
                             this->InCellInterpolationKeys,
                             this->InCellInterpolationInfo,
                             po,
                             this->CellMapOutputToInput);

    //}
    // Forthly, create the output set and clean up connectivity objects.
    //{
    // Get unique keys for all shared edges
    viskores::cont::Algorithm::SortByKey(
      edgeInterpolation, edgePointReverseConnectivity, EdgeInterpolation::LessThanOp());
    viskores::cont::Algorithm::Copy(edgeInterpolation, this->EdgePointsInterpolation);
    viskores::cont::Algorithm::Unique(this->EdgePointsInterpolation,
                                      EdgeInterpolation::EqualToOp());
    viskores::cont::ArrayHandle<viskores::Id> edgeInterpolationIndexToUnique;
    viskores::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                           edgeInterpolation,
                                           edgeInterpolationIndexToUnique,
                                           EdgeInterpolation::LessThanOp());

    viskores::cont::ArrayHandle<viskores::Id> cellInterpolationIndexToUnique;
    viskores::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                           cellPointEdgeInterpolation,
                                           cellInterpolationIndexToUnique,
                                           EdgeInterpolation::LessThanOp());
    this->EdgePointsOffset = cellSet.GetNumberOfPoints();
    this->InCellPointsOffset =
      this->EdgePointsOffset + this->EdgePointsInterpolation.GetNumberOfValues();

    ScatterEdgeConnectivity scatterEdgePointConnectivity(this->EdgePointsOffset);
    viskores::worklet::DispatcherMapField<ScatterEdgeConnectivity> scatterEdgeDispatcher(
      scatterEdgePointConnectivity);
    scatterEdgeDispatcher.Invoke(
      edgeInterpolationIndexToUnique, edgePointReverseConnectivity, connectivity);
    scatterEdgeDispatcher.Invoke(cellInterpolationIndexToUnique,
                                 cellPointEdgeReverseConnectivity,
                                 this->InCellInterpolationInfo);
    // Add offset in connectivity of all new in-cell points.
    ScatterInCellConnectivity scatterInCellPointConnectivity(this->InCellPointsOffset);
    viskores::worklet::DispatcherMapField<ScatterInCellConnectivity> scatterInCellDispatcher(
      scatterInCellPointConnectivity);
    scatterInCellDispatcher.Invoke(cellPointReverseConnectivity, connectivity);

    viskores::cont::CellSetExplicit<> output;
    viskores::Id numberOfPoints = cellSet.GetNumberOfPoints() +
      this->EdgePointsInterpolation.GetNumberOfValues() + total.NumberOfInCellPoints;

    viskores::cont::ConvertNumComponentsToOffsets(numberOfIndices, offset);
    // Create explicit cell set output
    output.Fill(numberOfPoints, shapes, connectivity, offset);
    //}
    viskores::cont::ArrayCopy(po.newCellColors, newIDs);
    viskores::cont::ArrayCopy(po.newCellLookback, newLookback);

    return output;
  }

  template <typename ArrayHandleType>
  class InterpolateField
  {
  public:
    using ValueType = typename ArrayHandleType::ValueType;
    using TypeMappedValue = viskores::List<ValueType>;

    InterpolateField(viskores::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                     viskores::cont::ArrayHandle<viskores::Id> inCellInterpolationKeys,
                     viskores::cont::ArrayHandle<viskores::Id> inCellInterpolationInfo,
                     viskores::Id edgePointsOffset,
                     viskores::Id inCellPointsOffset,
                     ArrayHandleType* output)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , Output(output)
    {
    }

    class PerformEdgeInterpolations : public viskores::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(viskores::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations, WholeArrayInOut outputField);

      using ExecutionSignature = void(_1, _2, WorkIndex);

      template <typename EdgeInterp, typename OutputFieldPortal>
      VISKORES_EXEC void operator()(const EdgeInterp& ei,
                                    OutputFieldPortal& field,
                                    const viskores::Id workIndex) const
      {
        using T = typename OutputFieldPortal::ValueType;
        T v1 = field.Get(ei.Vertex1);
        T v2 = field.Get(ei.Vertex2);
        field.Set(this->EdgePointsOffset + workIndex,
                  static_cast<T>(MIRinternal::Scale(T(v1 - v2), ei.Weight) + v2));
        if (ei.Weight > viskores::Float64(1) || ei.Weight < viskores::Float64(0))
        {
          this->RaiseError("Error in edge weight, assigned value not it interval [0,1].");
        }
      }

    private:
      viskores::Id EdgePointsOffset;
    };

    class PerformInCellInterpolations : public viskores::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys, ValuesIn toReduce, ReducedValuesOut centroid);

      using ExecutionSignature = void(_2, _3);

      template <typename MappedValueVecType, typename MappedValueType>
      VISKORES_EXEC void operator()(const MappedValueVecType& toReduce,
                                    MappedValueType& centroid) const
      {
        viskores::IdComponent numValues = toReduce.GetNumberOfComponents();
        MappedValueType sum = toReduce[0];
        for (viskores::IdComponent i = 1; i < numValues; i++)
        {
          MappedValueType value = toReduce[i];
          // static_cast is for when MappedValueType is a small int that gets promoted to int32.
          sum = static_cast<MappedValueType>(sum + value);
        }
        centroid = MIRinternal::Scale(sum, 1. / static_cast<viskores::Float64>(numValues));
      }
    };

    template <typename Storage>
    VISKORES_CONT void operator()(
      const viskores::cont::ArrayHandle<ValueType, Storage>& field) const
    {
      viskores::worklet::Keys<viskores::Id> interpolationKeys(InCellInterpolationKeys);

      viskores::Id numberOfOriginalValues = field.GetNumberOfValues();
      viskores::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();
      viskores::Id numberOfInCellPoints = interpolationKeys.GetUniqueKeys().GetNumberOfValues();

      ArrayHandleType result;
      result.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      viskores::cont::Algorithm::CopySubRange(field, 0, numberOfOriginalValues, result);

      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalValues);
      viskores::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, result);

      // Perform a gather on output to get all required values for calculation of
      // centroids using the interpolation info array.
      using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
      using ValueHandle = viskores::cont::ArrayHandle<ValueType>;
      viskores::cont::ArrayHandlePermutation<IdHandle, ValueHandle> toReduceValues(
        InCellInterpolationInfo, result);

      viskores::cont::ArrayHandle<ValueType> reducedValues;
      viskores::worklet::DispatcherReduceByKey<PerformInCellInterpolations>
        inCellInterpolationDispatcher;
      inCellInterpolationDispatcher.Invoke(interpolationKeys, toReduceValues, reducedValues);
      viskores::Id inCellPointsOffset = numberOfOriginalValues + numberOfEdgePoints;
      viskores::cont::Algorithm::CopySubRange(
        reducedValues, 0, reducedValues.GetNumberOfValues(), result, inCellPointsOffset);
      *(this->Output) = result;
    }

  private:
    viskores::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationKeys;
    viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationInfo;
    viskores::Id EdgePointsOffset;
    viskores::Id InCellPointsOffset;
    ArrayHandleType* Output;
  };

  template <typename IDLen, typename IDPos, typename IDList, typename VFList>
  class InterpolateMIRFields
  {
  public:
    InterpolateMIRFields(viskores::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                         viskores::cont::ArrayHandle<viskores::Id> inCellInterpolationKeys,
                         viskores::cont::ArrayHandle<viskores::Id> inCellInterpolationInfo,
                         viskores::Id edgePointsOffset,
                         viskores::Id inCellPointsOffset,
                         IDLen* output1,
                         IDPos* output2,
                         IDList* output3,
                         VFList* output4)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , LenOut(output1)
      , PosOut(output2)
      , IDOut(output3)
      , VFOut(output4)
    {
    }

    class PerformEdgeInterpolations : public viskores::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(viskores::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations,
                                    WholeArrayIn lengths,
                                    WholeArrayIn positions,
                                    WholeArrayInOut ids,
                                    WholeArrayInOut vfs);

      using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex);

      template <typename EdgeInterp, typename IDL, typename IDO, typename IdsVec, typename VfsVec>
      VISKORES_EXEC void operator()(const EdgeInterp& ei,
                                    const IDL& lengths,
                                    const IDO& positions,
                                    IdsVec& ids,
                                    VfsVec& vfs,
                                    const viskores::Id workIndex) const
      {
        viskores::Vec<viskores::Id, 2> idOff;
        viskores::Vec<viskores::Id, 2> idLen;
        viskores::Vec<viskores::Id, 2> idInd;
        viskores::Vec<viskores::Float64, 2> multiplier;
        multiplier[1] = viskores::Float64(1.0) - ei.Weight;
        multiplier[0] = ei.Weight;
        viskores::Id uniqueMats = viskores::Id(0);

        idOff[0] = viskores::Id(0);
        idOff[1] = idOff[0];
        idInd[0] = positions.Get(ei.Vertex1);
        idInd[1] = positions.Get(ei.Vertex2);
        idLen[0] = lengths.Get(ei.Vertex1);
        idLen[1] = lengths.Get(ei.Vertex2);
        viskores::IdComponent numberOfPoints = 2;
        viskores::UInt8 hasWork = viskores::UInt8(1);
        while (hasWork != viskores::UInt8(0))
        {
          hasWork = viskores::UInt8(0);
          viskores::Id lowest = viskores::Id(-1);
          for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == viskores::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = viskores::UInt8(1);
              }
            }
          }
          if (hasWork != viskores::UInt8(0))
          {
            viskores::Float64 vfVal = viskores::Float64(0);
            for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  vfVal += multiplier[i] * vfs.Get(idInd[i] + idOff[i]);
                  idOff[i]++;
                }
              }
            }
            ids.Set(positions.Get(this->EdgePointsOffset + workIndex) + uniqueMats, lowest);
            vfs.Set(positions.Get(this->EdgePointsOffset + workIndex) + uniqueMats, vfVal);
            uniqueMats++;
          }
        }
      }

    private:
      viskores::Id EdgePointsOffset;
    };
    class PerformEdgeInterpolations_C : public viskores::worklet::WorkletMapField
    {
    private:
      viskores::Id EdgePointsOffset;

    public:
      PerformEdgeInterpolations_C(viskores::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }
      using ControlSignature = void(FieldIn edgeInterpolations,
                                    WholeArrayInOut IDLengths,
                                    WholeArrayIn IDOffsets,
                                    WholeArrayIn IDs,
                                    FieldOut edgeLength);
      using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex, _5);
      template <typename EdgeInterp, typename IDL, typename IDO, typename IdsVec, typename ELL>
      VISKORES_EXEC void operator()(const EdgeInterp& ei,
                                    IDL& lengths,
                                    const IDO& positions,
                                    const IdsVec& ids,
                                    const viskores::Id workIndex,
                                    ELL& edgelength) const
      {
        viskores::Vec<viskores::Id, 2> idOff;
        viskores::Vec<viskores::Id, 2> idLen;
        viskores::Vec<viskores::Id, 2> idInd;
        viskores::Id uniqueMats = viskores::Id(0);

        idOff[0] = viskores::Id(0);
        idOff[1] = idOff[0];
        idInd[0] = positions.Get(ei.Vertex1);
        idInd[1] = positions.Get(ei.Vertex2);
        idLen[0] = lengths.Get(ei.Vertex1);
        idLen[1] = lengths.Get(ei.Vertex2);
        viskores::IdComponent numberOfPoints = 2;
        viskores::UInt8 hasWork = viskores::UInt8(1);
        while (hasWork != viskores::UInt8(0))
        {
          hasWork = viskores::UInt8(0);
          viskores::Id lowest = viskores::Id(-1);
          for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == viskores::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = viskores::UInt8(1);
              }
            }
          }
          if (hasWork != viskores::UInt8(0))
          {
            for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  idOff[i]++;
                }
              }
            }
            uniqueMats++;
          }
        }
        lengths.Set(this->EdgePointsOffset + workIndex, uniqueMats);
        edgelength = uniqueMats;
      }
    };

    class PerformInCellInterpolations_C : public viskores::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys,
                                    ValuesIn toReduce,
                                    WholeArrayIn IDLengths,
                                    WholeArrayIn IDOffsets,
                                    WholeArrayIn IDs,
                                    ReducedValuesOut centroid);

      using ExecutionSignature = void(_2, _3, _4, _5, _6);

      template <typename MappedValueVecType,
                typename MappedValueType,
                typename IDArr,
                typename IDOff,
                typename IdsVec>
      VISKORES_EXEC void operator()(const MappedValueVecType& toReduce,
                                    const IDArr& lengths,
                                    const IDOff& positions,
                                    const IdsVec& ids,
                                    MappedValueType& numIdNeeded) const
      {
        viskores::IdComponent numberOfPoints = toReduce.GetNumberOfComponents();
        // ToReduce is simply the indexArray, giving us point information (since this is reduce by key)
        // numIdNeeded is the output length of this key
        using IdVec = viskores::Vec<viskores::Id, 8>;
        IdVec idOff = viskores::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idLen = viskores::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idInd = viskores::TypeTraits<IdVec>::ZeroInitialization();
        viskores::Id uniqueMats = viskores::Id(0);

        for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
        {
          idOff[i] = 0;
          idLen[i] = lengths.Get(toReduce[i]);
          idInd[i] = positions.Get(toReduce[i]);
        }

        viskores::UInt8 hasWork = viskores::UInt8(1);
        while (hasWork != viskores::UInt8(0))
        {
          hasWork = viskores::UInt8(0);
          viskores::Id lowest = viskores::Id(-1);
          for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == viskores::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = viskores::UInt8(1);
              }
            }
          }
          if (hasWork != viskores::UInt8(0))
          {
            for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  idOff[i]++;
                }
              }
            }
            uniqueMats++;
          }
        }
        numIdNeeded = uniqueMats;
      }
    };

    class PerformInCellInterpolations : public viskores::worklet::WorkletReduceByKey
    {
    private:
      viskores::Id offset;

    public:
      PerformInCellInterpolations(viskores::Id outputOffsetForBookkeeping)
        : offset(outputOffsetForBookkeeping)
      {
      }
      using ControlSignature = void(KeysIn keys,
                                    ValuesIn toReduce,
                                    WholeArrayIn IDLengths,
                                    WholeArrayIn IDOffsets,
                                    WholeArrayIn IDs,
                                    WholeArrayIn VFs,
                                    ReducedValuesIn indexOff,
                                    ReducedValuesOut reindexedOut,
                                    WholeArrayOut outputIDs,
                                    WholeArrayOut outputVFs);

      using ExecutionSignature = void(_2, _3, _4, _5, _6, _7, _8, _9, _10);

      template <typename MappedValueVecType,
                typename IDArr,
                typename IDOff,
                typename IdsVec,
                typename VfsVec,
                typename IndexIn,
                typename IndexOut,
                typename OutID,
                typename OutVF>
      VISKORES_EXEC void operator()(const MappedValueVecType& toReduce,
                                    const IDArr& lengths,
                                    const IDOff& positions,
                                    const IdsVec& ids,
                                    const VfsVec& vfs,
                                    const IndexIn& localOffset,
                                    IndexOut& globalOffset,
                                    OutID& outIDs,
                                    OutVF& outVFs) const
      {

        globalOffset = localOffset + this->offset;
        viskores::IdComponent numberOfPoints = toReduce.GetNumberOfComponents();
        // ToReduce is simply the indexArray, giving us point information (since this is reduce by key)

        // numIdNeeded is the output length of this key
        using IdVec = viskores::Vec<viskores::Id, 8>;
        IdVec idOff = viskores::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idLen = viskores::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idInd = viskores::TypeTraits<IdVec>::ZeroInitialization();
        viskores::Id uniqueMats = viskores::Id(0);

        for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
        {
          idOff[i] = 0;
          idLen[i] = lengths.Get(toReduce[i]);
          idInd[i] = positions.Get(toReduce[i]);
        }

        viskores::UInt8 hasWork = viskores::UInt8(1);
        while (hasWork != viskores::UInt8(0))
        {
          hasWork = viskores::UInt8(0);
          viskores::Id lowest = viskores::Id(-1);
          for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == viskores::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = viskores::UInt8(1);
              }
            }
          }
          if (hasWork != viskores::UInt8(0))
          {
            viskores::Float64 val = viskores::Float64(0);
            for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                viskores::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  val += vfs.Get(idInd[i] + idOff[i]);
                  idOff[i]++;
                }
              }
            }
            outVFs.Set(localOffset + uniqueMats, val / viskores::Float64(numberOfPoints));
            outIDs.Set(localOffset + uniqueMats, lowest);
            uniqueMats++;
          }
        }
      }
    };

    VISKORES_CONT void operator()(
      const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& originalLen,
      const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& originalPos,
      const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& originalIDs,
      const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>&
        originalVFs) const
    {
      viskores::worklet::Keys<viskores::Id> interpolationKeys(InCellInterpolationKeys);
      viskores::Id numberOfOriginalPos = originalLen.GetNumberOfValues();
      viskores::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();

      viskores::cont::ArrayHandle<viskores::Id> lengthArr;
      viskores::cont::ArrayHandle<viskores::Id> posArr;
      viskores::cont::ArrayHandle<viskores::Id> idArr;
      viskores::cont::ArrayHandle<viskores::Float64> vfArr;
      lengthArr.Allocate(numberOfOriginalPos + numberOfEdgePoints);
      posArr.Allocate(numberOfOriginalPos + numberOfEdgePoints);
      viskores::cont::Algorithm::CopySubRange(originalLen, 0, numberOfOriginalPos, lengthArr);
      viskores::cont::Algorithm::CopySubRange(originalPos, 0, numberOfOriginalPos, posArr);

      viskores::cont::ArrayHandle<viskores::Id> edgeLengths;
      PerformEdgeInterpolations_C edgeCountWorklet(numberOfOriginalPos);
      viskores::worklet::DispatcherMapField<PerformEdgeInterpolations_C> edgeInterpDispatcher_C(
        edgeCountWorklet);
      edgeInterpDispatcher_C.Invoke(
        this->EdgeInterpolationArray, lengthArr, posArr, originalIDs, edgeLengths);

      viskores::Id idLengthFromJustEdges =
        viskores::cont::Algorithm::Reduce(edgeLengths, viskores::Id(0));

      idArr.Allocate(originalIDs.GetNumberOfValues() + idLengthFromJustEdges);
      vfArr.Allocate(originalIDs.GetNumberOfValues() + idLengthFromJustEdges);
      viskores::cont::Algorithm::CopySubRange(
        originalIDs, 0, originalIDs.GetNumberOfValues(), idArr);
      viskores::cont::Algorithm::CopySubRange(
        originalVFs, 0, originalIDs.GetNumberOfValues(), vfArr);
      viskores::cont::Algorithm::ScanExclusive(lengthArr, posArr);

      // Accept that you will have to copy data :| Maybe can speed this up with some special logic...
      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalPos);
      viskores::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, lengthArr, posArr, idArr, vfArr);

      // Need to run actual edgeInterpDispatcher, we then reduce the values


      viskores::cont::ArrayHandleIndex pointArr(numberOfOriginalPos + numberOfEdgePoints);
      viskores::cont::ArrayHandle<viskores::Id> pointArrCp;
      viskores::cont::ArrayCopy(pointArr, pointArrCp);
      using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
      using ValueHandle = viskores::cont::ArrayHandle<viskores::Id>;
      viskores::cont::ArrayHandlePermutation<IdHandle, ValueHandle> toReduceValues(
        InCellInterpolationInfo, pointArrCp);

      PerformInCellInterpolations_C incellCountWorklet;
      viskores::cont::ArrayHandle<viskores::Id> reducedIDCounts;
      viskores::worklet::DispatcherReduceByKey<PerformInCellInterpolations_C> cellCountDispatcher(
        incellCountWorklet);
      cellCountDispatcher.Invoke(
        interpolationKeys, toReduceValues, lengthArr, posArr, idArr, reducedIDCounts);

      viskores::cont::ArrayHandle<viskores::Id> reducedIDOffsets;
      viskores::Id totalIDLen =
        viskores::cont::Algorithm::ScanExclusive(reducedIDCounts, reducedIDOffsets);

      PerformInCellInterpolations incellWorklet(originalIDs.GetNumberOfValues() +
                                                idLengthFromJustEdges);
      viskores::cont::ArrayHandle<viskores::Id> cellids, cellOffsets;
      viskores::cont::ArrayHandle<viskores::Float64> cellvfs;

      cellids.Allocate(totalIDLen);
      cellvfs.Allocate(totalIDLen);
      viskores::worklet::DispatcherReduceByKey<PerformInCellInterpolations> cellInterpDispatcher(
        incellWorklet);
      cellInterpDispatcher.Invoke(interpolationKeys,
                                  toReduceValues,
                                  lengthArr,
                                  posArr,
                                  idArr,
                                  vfArr,
                                  reducedIDOffsets,
                                  cellOffsets,
                                  cellids,
                                  cellvfs);

      viskores::Id inCellVFOffset = originalIDs.GetNumberOfValues() + idLengthFromJustEdges;
      viskores::cont::Algorithm::CopySubRange(cellids, 0, totalIDLen, idArr, inCellVFOffset);
      viskores::cont::Algorithm::CopySubRange(cellvfs, 0, totalIDLen, vfArr, inCellVFOffset);
      viskores::cont::Algorithm::CopySubRange(reducedIDCounts,
                                              0,
                                              reducedIDCounts.GetNumberOfValues(),
                                              lengthArr,
                                              numberOfOriginalPos + numberOfEdgePoints);
      viskores::cont::Algorithm::CopySubRange(cellOffsets,
                                              0,
                                              cellOffsets.GetNumberOfValues(),
                                              posArr,
                                              numberOfOriginalPos + numberOfEdgePoints);

      *(this->LenOut) = lengthArr;
      *(this->PosOut) = posArr;
      *(this->IDOut) = idArr;
      *(this->VFOut) = vfArr;
    }

  private:
    viskores::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationKeys;
    viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationInfo;
    viskores::Id EdgePointsOffset;
    viskores::Id InCellPointsOffset;
    IDLen* LenOut;
    IDPos* PosOut;
    IDList* IDOut;
    VFList* VFOut;
  };

  template <typename LookbackArr, typename WeightArr>
  class InterpolateLookbackField
  {
  public:
    InterpolateLookbackField(viskores::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                             viskores::cont::ArrayHandle<viskores::Id> inCellInterpolationKeys,
                             viskores::cont::ArrayHandle<viskores::Id> inCellInterpolationInfo,
                             viskores::Id edgePointsOffset,
                             viskores::Id inCellPointsOffset,
                             LookbackArr* output,
                             WeightArr* output2)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , Output(output)
      , Output2(output2)
    {
    }
    class PerformEdgeInterpolations : public viskores::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(viskores::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations,
                                    WholeArrayInOut inoutID,
                                    WholeArrayInOut inoutWeights);

      using ExecutionSignature = void(_1, _2, _3, WorkIndex);

      template <typename EdgeInterp, typename InOutId, typename InOutWeight>
      VISKORES_EXEC void operator()(const EdgeInterp& ei,
                                    InOutId& field,
                                    InOutWeight& field1,
                                    const viskores::Id workIndex) const
      {

        viskores::Vec<viskores::IdComponent, 2> curOff;
        viskores::Vec<viskores::Float64, 2> mult;
        viskores::Vec<viskores::Id, 8> centroid;
        viskores::Vec<viskores::Float64, 8> weight;
        viskores::Vec<viskores::Vec<viskores::Id, 8>, 2> keys;
        viskores::Vec<viskores::Vec<viskores::Float64, 8>, 2> weights;
        keys[0] = field.Get(ei.Vertex1);
        keys[1] = field.Get(ei.Vertex2);
        weights[0] = field1.Get(ei.Vertex1);
        weights[1] = field1.Get(ei.Vertex2);
        for (viskores::IdComponent i = 0; i < 8; i++)
        {
          weight[i] = 0;
          centroid[i] = -1;
        }
        curOff[0] = 0;
        curOff[1] = 0;
        mult[0] = ei.Weight;
        mult[1] = viskores::Float64(1.0) - ei.Weight;
        for (viskores::IdComponent j = 0; j < 8; j++)
        {
          viskores::Id lowestID = viskores::Id(-1);
          for (viskores::IdComponent i = 0; i < 2; i++)
          {
            if (curOff[i] < 8 &&
                (lowestID == viskores::Id(-1) ||
                 ((keys[i])[curOff[i]] != viskores::Id(-1) && (keys[i])[curOff[i]] < lowestID)))
            {
              lowestID = (keys[i])[curOff[i]];
            }
            if (curOff[i] < 8 && (keys[i])[curOff[i]] == viskores::Id(-1))
            {
              curOff[i] = 8;
            }
          }
          if (lowestID == viskores::Id(-1))
          {
            break;
          }
          centroid[j] = lowestID;
          for (viskores::IdComponent i = 0; i < 2; i++)
          {
            if (curOff[i] < 8 && lowestID == (keys[i])[curOff[i]])
            {
              weight[j] += mult[i] * weights[i][curOff[i]];
              curOff[i]++;
            }
          }
        }

        field.Set(this->EdgePointsOffset + workIndex, centroid);
        field1.Set(this->EdgePointsOffset + workIndex, weight);
      }

    private:
      viskores::Id EdgePointsOffset;
    };

    class PerformInCellInterpolations : public viskores::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys,
                                    ValuesIn toReduceID,
                                    WholeArrayIn Keys,
                                    WholeArrayIn weights,
                                    ReducedValuesOut id,
                                    ReducedValuesOut weight);

      using ExecutionSignature = void(_2, _3, _4, _5, _6);

      template <typename IDs,
                typename VecOfVecIDs,
                typename VecOfVecWeights,
                typename VecId,
                typename VecWeight>
      VISKORES_EXEC void operator()(const IDs& ids,
                                    const VecOfVecIDs& keysIn,
                                    const VecOfVecWeights& weightsIn,
                                    VecId& centroid,
                                    VecWeight& weight) const
      {
        viskores::IdComponent numValues = ids.GetNumberOfComponents();
        viskores::Vec<viskores::IdComponent, 8> curOff;
        viskores::Vec<viskores::Vec<viskores::Id, 8>, 8> keys;
        viskores::Vec<viskores::Vec<viskores::Float64, 8>, 8> weights;
        for (viskores::IdComponent i = 0; i < 8; i++)
        {
          weight[i] = 0;
          centroid[i] = -1;
          curOff[i] = 0;
        }
        for (viskores::IdComponent i = 0; i < numValues; i++)
        {
          keys[i] = keysIn.Get(ids[i]);
          weights[i] = weightsIn.Get(ids[i]);
        }
        for (viskores::IdComponent i = numValues; i < 8; i++)
        {
          curOff[i] = 8;
        }
        for (viskores::IdComponent j = 0; j < 8; j++)
        {
          viskores::Id lowestID = viskores::Id(-1);
          for (viskores::IdComponent i = 0; i < numValues; i++)
          {
            auto tmp = keys[i];
            if (curOff[i] < 8 &&
                (lowestID == viskores::Id(-1) ||
                 (tmp[curOff[i]] != viskores::Id(-1) && tmp[curOff[i]] < lowestID)))
            {
              lowestID = tmp[curOff[i]];
            }

            if (curOff[i] < 8 && tmp[curOff[i]] == viskores::Id(-1))
            {
              curOff[i] = 8;
            }
          }
          if (lowestID == viskores::Id(-1))
          {
            break;
          }
          centroid[j] = lowestID;
          for (viskores::IdComponent i = 0; i < numValues; i++)
          {
            auto tmp = keys[i];
            if (curOff[i] < 8 && lowestID == tmp[curOff[i]])
            {
              auto w = weights[i];
              weight[j] += w[curOff[i]];
              curOff[i]++;
            }
          }
        }
        for (viskores::IdComponent j = 0; j < 8; j++)
        {
          weight[j] *= 1. / static_cast<viskores::Float64>(numValues);
          VISKORES_ASSERT(curOff[j] == 8);
        }
      }
    };

    template <typename ValueType, typename ValueType1, typename Storage, typename Storage2>
    VISKORES_CONT void operator()(
      const viskores::cont::ArrayHandle<ValueType, Storage>& fieldID,
      const viskores::cont::ArrayHandle<ValueType1, Storage2>& weightsField) const
    {
      viskores::worklet::Keys<viskores::Id> interpolationKeys(InCellInterpolationKeys);

      viskores::Id numberOfOriginalValues = fieldID.GetNumberOfValues();
      viskores::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();
      viskores::Id numberOfInCellPoints = interpolationKeys.GetUniqueKeys().GetNumberOfValues();
      LookbackArr result;
      result.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      viskores::cont::Algorithm::CopySubRange(fieldID, 0, numberOfOriginalValues, result);
      WeightArr result2;
      result2.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      viskores::cont::Algorithm::CopySubRange(weightsField, 0, numberOfOriginalValues, result2);

      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalValues);
      viskores::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, result, result2);

      // Perform a gather on output to get all required values for calculation of
      // centroids using the interpolation info array.oi
      viskores::cont::ArrayHandleIndex nout(numberOfOriginalValues + numberOfEdgePoints);
      auto toReduceValues = make_ArrayHandlePermutation(InCellInterpolationInfo, nout);

      viskores::cont::ArrayHandle<ValueType> reducedValues;
      viskores::cont::ArrayHandle<ValueType1> reducedWeights;
      viskores::worklet::DispatcherReduceByKey<PerformInCellInterpolations>
        inCellInterpolationDispatcher;
      inCellInterpolationDispatcher.Invoke(
        interpolationKeys, toReduceValues, result, result2, reducedValues, reducedWeights);
      viskores::Id inCellPointsOffset = numberOfOriginalValues + numberOfEdgePoints;
      viskores::cont::Algorithm::CopySubRange(
        reducedValues, 0, reducedValues.GetNumberOfValues(), result, inCellPointsOffset);
      viskores::cont::Algorithm::CopySubRange(
        reducedWeights, 0, reducedWeights.GetNumberOfValues(), result2, inCellPointsOffset);
      *(this->Output) = result;
      *(this->Output2) = result2;
    }

  private:
    viskores::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationKeys;
    viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationInfo;
    viskores::Id EdgePointsOffset;
    viskores::Id InCellPointsOffset;
    LookbackArr* Output;
    WeightArr* Output2;
  };
  void ProcessSimpleMIRField(
    const viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>,
                                      viskores::cont::StorageTagBasic>& orLookback,
    const viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>,
                                      viskores::cont::StorageTagBasic>& orWeights,
    viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>, viskores::cont::StorageTagBasic>&
      newLookback,
    viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>,
                                viskores::cont::StorageTagBasic>& newweights) const
  {
    auto worker =
      InterpolateLookbackField<viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 8>>,
                               viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 8>>>(
        this->EdgePointsInterpolation,
        this->InCellInterpolationKeys,
        this->InCellInterpolationInfo,
        this->EdgePointsOffset,
        this->InCellPointsOffset,
        &newLookback,
        &newweights);
    worker(orLookback, orWeights);
  }
  void ProcessMIRField(
    const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic> orLen,
    const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic> orPos,
    const viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic> orIDs,
    const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic> orVFs,
    viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& newLen,
    viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& newPos,
    viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& newIDs,
    viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& newVFs) const
  {
    auto worker = InterpolateMIRFields<
      viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>,
      viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>>(
      this->EdgePointsInterpolation,
      this->InCellInterpolationKeys,
      this->InCellInterpolationInfo,
      this->EdgePointsOffset,
      this->InCellPointsOffset,
      &newLen,
      &newPos,
      &newIDs,
      &newVFs);
    worker(orLen, orPos, orIDs, orVFs);
  }

  template <typename ValueType, typename StorageType>
  viskores::cont::ArrayHandle<ValueType> ProcessPointField(
    const viskores::cont::ArrayHandle<ValueType, StorageType>& fieldData) const
  {
    using ResultType = viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagBasic>;
    using Worker = InterpolateField<ResultType>;
    ResultType output;
    Worker worker = Worker(this->EdgePointsInterpolation,
                           this->InCellInterpolationKeys,
                           this->InCellInterpolationInfo,
                           this->EdgePointsOffset,
                           this->InCellPointsOffset,
                           &output);
    worker(fieldData);
    return output;
  }

private:
  MIRCases::MIRTables MIRTablesInstance;
  viskores::cont::ArrayHandle<EdgeInterpolation> EdgePointsInterpolation;
  viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationKeys;
  viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationInfo;
  viskores::cont::ArrayHandle<viskores::Id> CellMapOutputToInput;
  viskores::Id EdgePointsOffset;
  viskores::Id InCellPointsOffset;
};

}
}

namespace viskores
{
namespace worklet
{
template <typename IDType, typename FloatType>
struct MIRObject : public viskores::cont::ExecutionAndControlObjectBase
{
public:
  class MIRObjectPortal
  {
  public:
    VISKORES_EXEC FloatType GetVFForPoint(IDType point, IDType matID, IDType) const
    {
      IDType low = this->PPos.Get(point);
      IDType high = this->PPos.Get(point) + this->PLens.Get(point) - 1;
      IDType matIdAt = -1;
      while (low <= high)
      {
        IDType mid = (low + high) / 2;
        IDType midMatId = this->PIDs.Get(mid);
        if (matID == midMatId)
        {
          matIdAt = mid;
          break;
        }
        else if (matID > midMatId)
        {
          low = mid + 1;
        }
        else if (matID < midMatId)
        {
          high = mid - 1;
        }
      }
      if (matIdAt >= 0)
      {
        return this->PVFs.Get(matIdAt);
      }
      else
        return FloatType(0);
    }

  private:
    typename viskores::cont::ArrayHandle<IDType, viskores::cont::StorageTagBasic>::ReadPortalType
      PLens;
    typename viskores::cont::ArrayHandle<IDType, viskores::cont::StorageTagBasic>::ReadPortalType
      PPos;
    typename viskores::cont::ArrayHandle<IDType, viskores::cont::StorageTagBasic>::ReadPortalType
      PIDs;
    typename viskores::cont::ArrayHandle<FloatType, viskores::cont::StorageTagBasic>::ReadPortalType
      PVFs;
    friend struct MIRObject;
  };

  VISKORES_CONT viskores::cont::ArrayHandle<IDType> getPointLenArr() { return this->pointLen; }
  VISKORES_CONT viskores::cont::ArrayHandle<IDType> getPointPosArr() { return this->pointPos; }
  VISKORES_CONT viskores::cont::ArrayHandle<IDType> getPointIDArr() { return this->pointIDs; }
  VISKORES_CONT viskores::cont::ArrayHandle<FloatType> getPointVFArr() { return this->pointVFs; }

  // Do we need to copy these arrays?
  template <typename IDInput, typename FloatInput>
  MIRObject(const IDInput& len, const IDInput& pos, const IDInput& ids, const FloatInput& floats)
    : pointLen(len)
    , pointPos(pos)
    , pointIDs(ids)
    , pointVFs(floats)
  {
  }

  MIRObjectPortal PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                      viskores::cont::Token& token)
  {
    MIRObjectPortal portal;
    portal.PLens = this->pointLen.PrepareForInput(device, token);
    portal.PPos = this->pointPos.PrepareForInput(device, token);
    portal.PIDs = this->pointIDs.PrepareForInput(device, token);
    portal.PVFs = this->pointVFs.PrepareForInput(device, token);
    return portal;
  }

private:
  viskores::cont::ArrayHandle<IDType> pointLen, pointPos, pointIDs;
  viskores::cont::ArrayHandle<FloatType> pointVFs;
};

struct CombineVFsForPoints_C : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInCell lens,
                                FieldInCell pos,
                                WholeArrayIn ids,
                                FieldOutPoint idcount);
  using ExecutionSignature = void(CellCount, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename LenVec, typename PosVec, typename IdsVec, typename OutVec>
  VISKORES_EXEC void operator()(viskores::IdComponent numCells,
                                const LenVec& len,
                                const PosVec& pos,
                                const IdsVec& ids,
                                OutVec& outlength) const
  {

    // This is for the number of VFs in the surrounding cells...
    // We assume that the ids are sorted.
    outlength = viskores::Id(0);

    viskores::Id uniqueMats = viskores::Id(0);

    using ida = viskores::Id;

    ida lowest = ids.Get(pos[0]);
    ida prevLowest = ida(-1);
    ida largest = ida(-1);

    for (viskores::IdComponent ci = 0; ci < numCells; ci++)
    {
      viskores::IdComponent l = viskores::IdComponent(pos[ci] + len[ci]);
      for (viskores::IdComponent idi = viskores::IdComponent(pos[ci]); idi < l; idi++)
      {
        ida tmp = ids.Get(idi);
        largest = viskores::Maximum()(tmp, largest);
      }
    }

    while (prevLowest != lowest)
    {
      for (viskores::IdComponent ci = 0; ci < numCells; ci++)
      {
        viskores::IdComponent l = viskores::IdComponent(pos[ci] + len[ci]);
        for (viskores::IdComponent idi = viskores::IdComponent(pos[ci]); idi < l; idi++)
        {
          ida tmp = ids.Get(idi);
          if (tmp < lowest && tmp > prevLowest)
          {
            lowest = tmp;
          }
        }
      }
      uniqueMats++;
      prevLowest = ida(lowest);
      lowest = ida(largest);
    }
    outlength = uniqueMats;
  }
};

struct CombineVFsForPoints : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInCell lens,
                                FieldInCell pos,
                                WholeArrayIn ids,
                                WholeArrayIn vfs,
                                FieldInPoint actpos,
                                WholeArrayOut idx,
                                WholeArrayOut vfx);
  using ExecutionSignature = void(CellCount, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;
  template <typename LenVec,
            typename PosVec,
            typename IdsVec,
            typename VfsVec,
            typename PosVec2,
            typename OutVec,
            typename OutVec2>
  VISKORES_EXEC void operator()(viskores::IdComponent numCells,
                                const LenVec& len,
                                const PosVec& pos,
                                const IdsVec& ids,
                                const VfsVec& vfs,
                                const PosVec2& posit,
                                OutVec& outid,
                                OutVec2& outvf) const
  {

    // This is for the number of VFs in the surrounding cells...
    // We assume that the ids are sorted.


    viskores::Id uniqueMats = viskores::Id(0);

    using ida = viskores::Id;

    ida lowest = ids.Get(pos[0]);
    ida prevLowest = ida(-1);
    ida largest = ida(-1);

    for (viskores::IdComponent ci = 0; ci < numCells; ci++)
    {
      viskores::IdComponent l = viskores::IdComponent(pos[ci] + len[ci]);
      for (viskores::IdComponent idi = viskores::IdComponent(pos[ci]); idi < l; idi++)
      {
        ida tmp = ids.Get(idi);
        largest = viskores::Maximum()(tmp, largest);
      }
    }

    while (prevLowest != lowest)
    {
      for (viskores::IdComponent ci = 0; ci < numCells; ci++)
      {
        viskores::IdComponent l = viskores::IdComponent(pos[ci] + len[ci]);
        for (viskores::IdComponent idi = viskores::IdComponent(pos[ci]); idi < l; idi++)
        {
          ida tmp = ids.Get(idi);
          if (tmp < lowest && tmp > prevLowest)
          {
            lowest = tmp;
          }
        }
      }
      outid.Set(posit + uniqueMats, lowest);
      viskores::Float64 avg = viskores::Float64(0);
      for (viskores::IdComponent ci = 0; ci < numCells; ci++)
      {
        viskores::IdComponent l = viskores::IdComponent(pos[ci] + len[ci]);
        for (viskores::IdComponent idi = viskores::IdComponent(pos[ci]); idi < l; idi++)
        {
          ida tmp = ids.Get(idi);
          if (tmp == lowest)
          {
            avg += vfs.Get(idi);
          }
        }
      }
      outvf.Set(posit + uniqueMats, avg / viskores::Float64(numCells));
      uniqueMats++;
      prevLowest = ida(lowest);
      lowest = ida(largest);
    }
  }
};

struct ExtractVFsForMIR_C : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell numPointsCount);
  using ExecutionSignature = void(PointCount, _2);
  using InputDomain = _1;
  template <typename OutVec>
  VISKORES_EXEC void operator()(viskores::IdComponent numPoints, OutVec& outlength) const
  {
    outlength = numPoints;
  }
};

struct ExtractVFsForMIR : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                ExecObject mir_obj,
                                FieldInCell previousMatID,
                                FieldOutCell curvfVals,
                                FieldOutCell prevvfVals);

  using ExecutionSignature = void(PointCount, VisitIndex, PointIndices, _2, _3, _4, _5);
  using InputDomain = _1;
  using ScatterType = viskores::worklet::ScatterCounting;
  template <typename CountArrayType>
  VISKORES_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
  {
    VISKORES_IS_ARRAY_HANDLE(CountArrayType);
    return ScatterType(countArray);
  }
  template <typename DA, typename prevID, typename OutVec, typename OutVec2, typename pointVec>
  VISKORES_EXEC void operator()(viskores::IdComponent numPoints,
                                viskores::IdComponent index,
                                pointVec& pointIDs,
                                const DA& mirobj,
                                const prevID& previousID,
                                OutVec& outVF,
                                OutVec2& prevOutVF) const
  {
    (void)numPoints;
    outVF = OutVec(0);
    prevOutVF = OutVec2(0);
    outVF = mirobj.GetVFForPoint(pointIDs[index], this->target, 0);
    if (previousID == 0)
    {
      prevOutVF = 0;
    }
    else
    {
      prevOutVF = mirobj.GetVFForPoint(pointIDs[index], previousID, 0);
    }
  }
  ExtractVFsForMIR(viskores::Id targetMat)
    : target(targetMat)
  {
  }

private:
  viskores::Id target;
};

struct CalcVol : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                ExecObject mirTables,
                                FieldInPoint verts,
                                FieldOutCell vol);
  using ExecutionSignature = void(PointCount, CellShape, _2, _3, _4);
  template <typename Arrout, typename PointListIn, typename Dev, typename CellShape>
  VISKORES_EXEC void operator()(const viskores::IdComponent pointCount,
                                const CellShape& cellShape,
                                const Dev& mirTable,
                                const PointListIn& vertPos,
                                Arrout& volumeOut) const
  {
    viskores::IdComponent numFaces =
      mirTable.GetNumberOfFaces(static_cast<viskores::Id>(cellShape.Id));

    viskores::Float64 totVol = viskores::Float64(0);
    viskores::IdComponent offset = mirTable.GetFaceOffset(static_cast<viskores::Id>(cellShape.Id));

    auto av1 = vertPos[0];
    for (viskores::IdComponent i = 1; i < pointCount; i++)
    {
      av1 += vertPos[i];
    }
    auto av = av1 * (viskores::Float64(1.0) / viskores::Float64(pointCount));

    for (viskores::IdComponent i = 0; i < numFaces; i++)
    {
      viskores::UInt8 p1 = mirTable.GetPoint(offset++);
      viskores::UInt8 p2 = mirTable.GetPoint(offset++);
      viskores::UInt8 p3 = mirTable.GetPoint(offset++);
      auto v1 = vertPos[p1];
      auto v2 = vertPos[p2];
      auto v3 = vertPos[p3];

      auto v4 = v1 - av;
      auto v5 = v2 - av;
      auto v6 = v3 - av;
      totVol += viskores::Abs(viskores::Dot(v4, viskores::Cross(v5, v6))) / 6;
    }
    volumeOut = totVol;
  }
};

struct CalcError_C : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn cellID,
                                ValuesIn cellColors,
                                WholeArrayIn origLen,
                                WholeArrayIn origPos,
                                WholeArrayIn origIDs,
                                WholeArrayOut newlengthsOut);
  using ExecutionSignature = void(ValueCount, _1, _2, _3, _4, _5, _6);
  using InputDomain = _1;
  template <typename Colors, typename ORL, typename ORP, typename ORID, typename NLO>
  VISKORES_EXEC void operator()(const viskores::IdComponent numCells,
                                const viskores::Id cellID,
                                const Colors& cellCol,
                                const ORL& orgLen,
                                const ORP& orgPos,
                                const ORID& orgID,
                                NLO& outputLen) const
  {
    // Although I don't doubt for a minute that keys is sorted and hence the output would be too,
    // but this ensures I don't deal with a headache if they change that.
    // The orgLen and orgPos are the true, original cell IDs and VFs
    // Luckily indexing into cellID should be quick compared to orgLen...
    viskores::Id lowest = orgID.Get(orgPos.Get(0));
    viskores::Id originalInd = 0;
    viskores::Id orgLen1 = orgLen.Get(cellID);
    viskores::Id orgPos1 = orgPos.Get(cellID);
    viskores::Id uniqueMats = 0;
    viskores::Id largest = orgID.Get(orgLen1 + orgPos1 - 1);
    for (viskores::IdComponent i = 0; i < numCells; i++)
    {
      viskores::Id tmp = cellCol[i];
      largest = viskores::Maximum()(tmp, largest);
    }
    viskores::Id prevLowest = viskores::Id(-1);
    lowest = viskores::Id(0);
    while (prevLowest != largest)
    {
      if (originalInd < orgLen1)
      {
        lowest = orgID.Get(orgPos1 + originalInd);
      }
      for (viskores::IdComponent i = 0; i < numCells; i++)
      {
        viskores::Id tmp = cellCol[i];
        if (tmp > prevLowest)
        {
          lowest = viskores::Minimum()(tmp, lowest);
        }
      }
      if (originalInd < orgLen1)
      {
        if (orgID.Get(orgPos1 + originalInd) == lowest)
        {
          originalInd++;
        }
      }
      uniqueMats++;

      prevLowest = lowest;
      lowest = largest;
    }
    outputLen.Set(cellID, uniqueMats);
  }
};

struct CalcError : public viskores::worklet::WorkletReduceByKey
{
private:
  viskores::Float64 lerping;

public:
  CalcError(viskores::Float64 errorLerp)
    : lerping(errorLerp)
  {
  }
  using ControlSignature = void(KeysIn cellID,
                                ValuesIn cellColors,
                                ValuesIn cellVols,
                                WholeArrayIn origLen,
                                WholeArrayIn origPos,
                                WholeArrayIn origIDs,
                                WholeArrayIn origVFs,
                                WholeArrayIn curLen,
                                WholeArrayIn curPos,
                                WholeArrayIn curID,
                                WholeArrayIn curVF,
                                WholeArrayIn newLength,
                                WholeArrayIn newposition,
                                WholeArrayOut newIDs,
                                WholeArrayOut newVFs,
                                WholeArrayIn origVols,
                                ReducedValuesOut totalErr);
  using ExecutionSignature =
    void(ValueCount, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17);
  using InputDomain = _1;
  template <typename Colors,
            typename ORL,
            typename ORP,
            typename ORID,
            typename NLO,
            typename ORVF,
            typename NID,
            typename NVF,
            typename Vols,
            typename TEO,
            typename CPos,
            typename CLen,
            typename CID,
            typename CVF,
            typename NLen,
            typename OVols>
  VISKORES_EXEC void operator()(const viskores::IdComponent numCells,
                                const viskores::Id cellID,
                                const Colors& cellCol,
                                const Vols& cellVolumes,
                                const ORL& orgLen,
                                const ORP& orgPos,
                                const ORID& orgID,
                                const ORVF& orgVF,
                                const CLen& curLen,
                                const CPos& curPos,
                                const CID& curID,
                                const CVF& curVF,
                                const NLen&,
                                const NLO& inputPos,
                                NID& inputIDs,
                                NVF& inputVFs,
                                const OVols& orgVols,
                                TEO& totalErrorOut) const
  {
    // Although I don't doubt for a minute that keys is sorted and hence the output would be too,
    // but this ensures I don't deal with a headache if they change that.
    // The orgLen and orgPos are the true, original cell IDs and VFs
    // Luckily indexing into cellID should be quick compared to orgLen...
    //{
    viskores::Id lowest = orgID.Get(orgPos.Get(0));
    viskores::Id originalInd = 0;
    viskores::Id orgLen1 = orgLen.Get(cellID);
    viskores::Id orgPos1 = orgPos.Get(cellID);
    viskores::Id uniqueMats = 0;
    viskores::Id largest = orgID.Get(orgLen1 + orgPos1 - 1);

    //viskores::Id canConnect = viskores::Id(0);
    for (viskores::IdComponent i = 0; i < numCells; i++)
    {
      viskores::Id tmp = cellCol[i];
      largest = viskores::Maximum()(tmp, largest);
    }
    viskores::Id prevLowest = viskores::Id(-1);

    viskores::Id currentIndex = curPos.Get(cellID);
    viskores::Id currentLens = curLen.Get(cellID) + currentIndex;
    //}

    viskores::Float64 totalError = viskores::Float64(0);
    while (prevLowest != largest)
    {
      if (originalInd < orgLen1)
      {
        lowest = orgID.Get(orgPos1 + originalInd);
      }
      for (viskores::IdComponent i = 0; i < numCells; i++)
      {
        viskores::Id tmp = cellCol[i];
        if (tmp > prevLowest)
        {
          lowest = viskores::Minimum()(tmp, lowest);
        }
      }
      viskores::Float64 totalVolForColor = viskores::Float64(0);
      for (viskores::IdComponent i = 0; i < numCells; i++)
      {
        viskores::Id tmp = cellCol[i];
        if (tmp == lowest)
        {
          totalVolForColor += cellVolumes[i];
        }
      }
      if (originalInd < orgLen1)
      {
        if (orgID.Get(orgPos1 + originalInd) == lowest)
        {
          totalVolForColor -= orgVF.Get(orgPos1 + originalInd) * orgVols.Get(cellID);
          originalInd++;
        }
      }

      viskores::Float64 prevTarget = viskores::Float64(0);
      if (currentIndex < currentLens)
      {

        if (curID.Get(currentIndex) == lowest)
        {
          prevTarget = curVF.Get(currentIndex);
          currentIndex++;
        }
      }
      //viskores::Float64 tmp = prevTarget;
      prevTarget += this->lerping * (-totalVolForColor) / orgVols.Get(cellID);
      totalError += viskores::Abs(totalVolForColor);
      //VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "Lerping " << tmp << " -> " << prevTarget << " :| " << totalVolForColor);
      //VISKORES_LOG_S(viskores::cont::LogLevel::Info, cellID << ": " << uniqueMats << " PT: " << tmp << " AVPTR "
      //            << totalVolForColor << " L: " << this->lerping << " and " << prevTarget << " / " << totalError
      //            << "\n" << inputPos.Get(cellID));
      inputIDs.Set(inputPos.Get(cellID) + uniqueMats, lowest);
      inputVFs.Set(inputPos.Get(cellID) + uniqueMats, viskores::FloatDefault(prevTarget));
      uniqueMats++;

      prevLowest = lowest;
      lowest = largest;
    }
    totalErrorOut = TEO(totalError);
  }
};

struct ConstructCellWeightList : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn pointIDs, FieldOut VecLookback, FieldOut VecWeights);
  using ExecutionSignature = void(InputIndex, _2, _3);
  using InputDomain = _1;
  template <typename VO1, typename VO2>
  VISKORES_EXEC void operator()(viskores::Id& in, VO1& lookback, VO2& weights) const
  {
    for (viskores::IdComponent i = 0; i < 8; i++)
    {
      lookback[i] = viskores::Id(-1);
      weights[i] = viskores::Float64(0);
    }
    lookback[0] = in;
    weights[0] = viskores::Float64(1);
  }
};

struct DestructPointWeightList : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn pointIDs,
                                FieldIn pointWeights,
                                WholeArrayIn originalVals,
                                FieldOut newVals);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;
  template <typename PID, typename PW, typename OV, typename NV>
  VISKORES_EXEC void operator()(const PID& pointIDs,
                                const PW& pointWeights,
                                const OV& originalVals,
                                NV& newVal) const
  {
    // This code assumes that originalVals and newVals come from ArrayHandleRecombineVec.
    // This means that they will have Vec-like values that support Vec operations. It also
    // means that operations have to be component-wise.
    VISKORES_ASSERT(pointIDs[0] != -1);
    using WeightType = typename PW::ComponentType;
    using ValueType = typename NV::ComponentType;
    auto originalVal = originalVals.Get(pointIDs[0]);
    for (viskores::IdComponent cIndex = 0; cIndex < newVal.GetNumberOfComponents(); ++cIndex)
    {
      newVal[cIndex] =
        static_cast<ValueType>(static_cast<WeightType>(originalVal[cIndex]) * pointWeights[0]);
    }
    for (viskores::IdComponent i = 1; i < 8; i++)
    {
      if (pointIDs[i] == viskores::Id(-1))
      {
        break;
      }
      else
      {
        originalVal = originalVals.Get(pointIDs[i]);
        for (viskores::IdComponent cIndex = 0; cIndex < newVal.GetNumberOfComponents(); ++cIndex)
        {
          newVal[cIndex] +=
            static_cast<ValueType>(static_cast<WeightType>(originalVal[cIndex]) * pointWeights[i]);
        }
      }
    }
  }
};

}

}

#endif
