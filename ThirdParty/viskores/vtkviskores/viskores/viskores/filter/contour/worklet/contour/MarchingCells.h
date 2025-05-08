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

#ifndef viskores_worklet_contour_MarchingCells_h
#define viskores_worklet_contour_MarchingCells_h

#include <viskores/BinaryPredicates.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/exec/CellDerivative.h>
#include <viskores/exec/CellEdge.h>
#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/ScatterPermutation.h>

#include <viskores/filter/contour/worklet/contour/CommonState.h>
#include <viskores/filter/contour/worklet/contour/FieldPropagation.h>
#include <viskores/filter/contour/worklet/contour/MarchingCellTables.h>
#include <viskores/filter/vector_analysis/worklet/gradient/PointGradient.h>
#include <viskores/filter/vector_analysis/worklet/gradient/StructuredPointGradient.h>
#include <viskores/worklet/WorkletReduceByKey.h>

namespace viskores
{
namespace worklet
{
namespace marching_cells
{

// -----------------------------------------------------------------------------
template <typename S>
viskores::cont::ArrayHandle<viskores::Float32, S> make_ScalarField(
  const viskores::cont::ArrayHandle<viskores::Float32, S>& ah)
{
  return ah;
}

template <typename S>
viskores::cont::ArrayHandle<viskores::Float64, S> make_ScalarField(
  const viskores::cont::ArrayHandle<viskores::Float64, S>& ah)
{
  return ah;
}

template <typename S>
viskores::cont::ArrayHandleCast<viskores::FloatDefault,
                                viskores::cont::ArrayHandle<viskores::UInt8, S>>
make_ScalarField(const viskores::cont::ArrayHandle<viskores::UInt8, S>& ah)
{
  return viskores::cont::make_ArrayHandleCast(ah, viskores::FloatDefault());
}

template <typename S>
viskores::cont::ArrayHandleCast<viskores::FloatDefault,
                                viskores::cont::ArrayHandle<viskores::Int8, S>>
make_ScalarField(const viskores::cont::ArrayHandle<viskores::Int8, S>& ah)
{
  return viskores::cont::make_ArrayHandleCast(ah, viskores::FloatDefault());
}

// ---------------------------------------------------------------------------
template <viskores::UInt8 InCellDim>
struct OutCellTraits;

template <>
struct OutCellTraits<3>
{
  static constexpr viskores::UInt8 NUM_POINTS = 3;
  static constexpr viskores::UInt8 CELL_SHAPE = viskores::CELL_SHAPE_TRIANGLE;
};

template <>
struct OutCellTraits<2>
{
  static constexpr viskores::UInt8 NUM_POINTS = 2;
  static constexpr viskores::UInt8 CELL_SHAPE = viskores::CELL_SHAPE_LINE;
};

template <>
struct OutCellTraits<1>
{
  static constexpr viskores::UInt8 NUM_POINTS = 1;
  static constexpr viskores::UInt8 CELL_SHAPE = viskores::CELL_SHAPE_VERTEX;
};

template <viskores::UInt8 Dims, typename FieldType, typename FieldVecType>
VISKORES_EXEC viskores::IdComponent TableNumOutCells(viskores::UInt8 shape,
                                                     FieldType isoValue,
                                                     const FieldVecType& fieldIn)
{
  const viskores::IdComponent numPoints = fieldIn.GetNumberOfComponents();
  // Compute the Marching Cubes case number for this cell. We need to iterate
  // the isovalues until the sum >= our visit index. But we need to make
  // sure the caseNumber is correct before stopping
  viskores::IdComponent caseNumber = 0;
  for (viskores::IdComponent point = 0; point < numPoints; ++point)
  {
    caseNumber |= (fieldIn[point] > isoValue) << point;
  }

  return viskores::worklet::marching_cells::GetNumOutCells<Dims>(shape, caseNumber);
}

template <typename FieldType, typename FieldVecType>
VISKORES_EXEC viskores::IdComponent NumOutCellsSpecialCases(
  std::integral_constant<viskores::UInt8, 3>,
  viskores::UInt8 shape,
  FieldType isoValue,
  const FieldVecType& fieldIn)
{
  return TableNumOutCells<3>(shape, isoValue, fieldIn);
}

template <typename FieldType, typename FieldVecType>
VISKORES_EXEC viskores::IdComponent NumOutCellsSpecialCases(
  std::integral_constant<viskores::UInt8, 2>,
  viskores::UInt8 shape,
  FieldType isoValue,
  const FieldVecType& fieldIn)
{
  if (shape == viskores::CELL_SHAPE_POLYGON)
  {
    const viskores::IdComponent numPoints = fieldIn.GetNumberOfComponents();
    viskores::IdComponent numCrossings = 0;
    bool lastOver = (fieldIn[numPoints - 1] > isoValue);
    for (viskores::IdComponent point = 0; point < numPoints; ++point)
    {
      bool nextOver = (fieldIn[point] > isoValue);
      if (lastOver != nextOver)
      {
        ++numCrossings;
      }
      lastOver = nextOver;
    }
    VISKORES_ASSERT((numCrossings % 2) == 0);
    return numCrossings / 2;
  }
  else
  {
    return TableNumOutCells<2>(shape, isoValue, fieldIn);
  }
}

template <typename FieldType, typename FieldVecType>
VISKORES_EXEC viskores::IdComponent NumOutCellsSpecialCases(
  std::integral_constant<viskores::UInt8, 1>,
  viskores::UInt8 shape,
  FieldType isoValue,
  const FieldVecType& fieldIn)
{
  if ((shape == viskores::CELL_SHAPE_LINE) || (shape == viskores::CELL_SHAPE_POLY_LINE))
  {
    const viskores::IdComponent numPoints = fieldIn.GetNumberOfComponents();
    viskores::IdComponent numCrossings = 0;
    bool lastOver = (fieldIn[0] > isoValue);
    for (viskores::IdComponent point = 1; point < numPoints; ++point)
    {
      bool nextOver = (fieldIn[point] > isoValue);
      if (lastOver != nextOver)
      {
        ++numCrossings;
      }
      lastOver = nextOver;
    }
    return numCrossings;
  }
  else
  {
    return 0;
  }
}

// ---------------------------------------------------------------------------
template <viskores::UInt8 Dims, typename T>
class ClassifyCell : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(WholeArrayIn isovalues,
                                FieldInPoint fieldIn,
                                CellSetIn cellSet,
                                FieldOutCell outNumTriangles);
  using ExecutionSignature = void(CellShape, _1, _2, _4);
  using InputDomain = _3;

  template <typename CellShapeType, typename IsoValuesType, typename FieldInType>
  VISKORES_EXEC void operator()(CellShapeType shape,
                                const IsoValuesType& isovalues,
                                const FieldInType& fieldIn,
                                viskores::IdComponent& numTriangles) const
  {
    viskores::IdComponent sum = 0;
    viskores::IdComponent numIsoValues =
      static_cast<viskores::IdComponent>(isovalues.GetNumberOfValues());

    for (viskores::Id i = 0; i < numIsoValues; ++i)
    {
      sum += NumOutCellsSpecialCases(
        std::integral_constant<viskores::UInt8, Dims>{}, shape.Id, isovalues.Get(i), fieldIn);
    }
    numTriangles = sum;
  }
};

/// \brief Used to store data need for the EdgeWeightGenerate worklet.
/// This information is not passed as part of the arguments to the worklet as
/// that dramatically increase compile time by 200%
// TODO: remove unused data members.
// -----------------------------------------------------------------------------
class EdgeWeightGenerateMetaData : viskores::cont::ExecutionObjectBase
{
public:
  class ExecObject
  {
    template <typename FieldType>
    using ReadPortalType = typename viskores::cont::ArrayHandle<FieldType>::ReadPortalType;
    template <typename FieldType>
    using WritePortalType = typename viskores::cont::ArrayHandle<FieldType>::WritePortalType;

  public:
    ExecObject() = default;

    VISKORES_CONT
    ExecObject(viskores::UInt8 numPointsPerOutCell,
               viskores::Id size,
               viskores::cont::ArrayHandle<viskores::FloatDefault>& interpWeights,
               viskores::cont::ArrayHandle<viskores::Id2>& interpIds,
               viskores::cont::ArrayHandle<viskores::Id>& interpCellIds,
               viskores::cont::ArrayHandle<viskores::UInt8>& interpContourId,
               viskores::cont::DeviceAdapterId device,
               viskores::cont::Token& token)
      : InterpWeightsPortal(
          interpWeights.PrepareForOutput(numPointsPerOutCell * size, device, token))
      , InterpIdPortal(interpIds.PrepareForOutput(numPointsPerOutCell * size, device, token))
      , InterpCellIdPortal(
          interpCellIds.PrepareForOutput(numPointsPerOutCell * size, device, token))
      , InterpContourPortal(
          interpContourId.PrepareForOutput(numPointsPerOutCell * size, device, token))
    {
      // Interp needs to be scaled as they are per point of the output cell
    }
    WritePortalType<viskores::FloatDefault> InterpWeightsPortal;
    WritePortalType<viskores::Id2> InterpIdPortal;
    WritePortalType<viskores::Id> InterpCellIdPortal;
    WritePortalType<viskores::UInt8> InterpContourPortal;
  };

  VISKORES_CONT
  EdgeWeightGenerateMetaData(viskores::UInt8 inCellDimension,
                             viskores::Id size,
                             viskores::cont::ArrayHandle<viskores::FloatDefault>& interpWeights,
                             viskores::cont::ArrayHandle<viskores::Id2>& interpIds,
                             viskores::cont::ArrayHandle<viskores::Id>& interpCellIds,
                             viskores::cont::ArrayHandle<viskores::UInt8>& interpContourId)
    : NumPointsPerOutCell(inCellDimension)
    , Size(size)
    , InterpWeights(interpWeights)
    , InterpIds(interpIds)
    , InterpCellIds(interpCellIds)
    , InterpContourId(interpContourId)
  {
  }

  VISKORES_CONT ExecObject PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                               viskores::cont::Token& token)
  {
    return ExecObject(this->NumPointsPerOutCell,
                      this->Size,
                      this->InterpWeights,
                      this->InterpIds,
                      this->InterpCellIds,
                      this->InterpContourId,
                      device,
                      token);
  }

private:
  viskores::UInt8 NumPointsPerOutCell;
  viskores::Id Size;
  viskores::cont::ArrayHandle<viskores::FloatDefault> InterpWeights;
  viskores::cont::ArrayHandle<viskores::Id2> InterpIds;
  viskores::cont::ArrayHandle<viskores::Id> InterpCellIds;
  viskores::cont::ArrayHandle<viskores::UInt8> InterpContourId;
};

// -----------------------------------------------------------------------------
template <viskores::UInt8 Dims, typename IsoValuesType, typename FieldVecType>
VISKORES_EXEC const viskores::UInt8* TableCellEdges(viskores::UInt8 shape,
                                                    const IsoValuesType& isoValues,
                                                    const FieldVecType& fieldIn,
                                                    viskores::IdComponent visitIndex,
                                                    viskores::IdComponent& contourIndex)
{
  const viskores::IdComponent numPoints = fieldIn.GetNumberOfComponents();
  // Compute the Marching Cubes case number for this cell. We need to iterate
  // the isovalues until the sum >= our visit index. But we need to make
  // sure the caseNumber is correct before stopping
  viskores::IdComponent caseNumber = 0;
  viskores::IdComponent sum = 0;
  viskores::IdComponent numIsoValues =
    static_cast<viskores::IdComponent>(isoValues.GetNumberOfValues());

  for (contourIndex = 0; contourIndex < numIsoValues; ++contourIndex)
  {
    const auto value = isoValues.Get(contourIndex);
    caseNumber = 0;
    for (viskores::IdComponent point = 0; point < numPoints; ++point)
    {
      caseNumber |= (fieldIn[point] > value) << point;
    }

    sum += viskores::worklet::marching_cells::GetNumOutCells<Dims>(shape, caseNumber);
    if (sum > visitIndex)
    {
      break;
    }
  }

  VISKORES_ASSERT(contourIndex < numIsoValues);

  visitIndex = sum - visitIndex - 1;

  return viskores::worklet::marching_cells::GetCellEdges<Dims>(shape, caseNumber, visitIndex);
}

template <typename IsoValuesType, typename FieldVecType>
VISKORES_EXEC const viskores::UInt8* CellEdgesSpecialCases(
  std::integral_constant<viskores::UInt8, 3>,
  viskores::UInt8 shape,
  const IsoValuesType& isoValues,
  const FieldVecType& fieldIn,
  viskores::IdComponent visitIndex,
  viskores::IdComponent& contourIndex,
  viskores::Vec2ui_8& viskoresNotUsed(edgeBuffer))
{
  return TableCellEdges<3>(shape, isoValues, fieldIn, visitIndex, contourIndex);
}

template <typename IsoValuesType, typename FieldVecType>
VISKORES_EXEC const viskores::UInt8* CellEdgesSpecialCases(
  std::integral_constant<viskores::UInt8, 2>,
  viskores::UInt8 shape,
  const IsoValuesType& isoValues,
  const FieldVecType& fieldIn,
  viskores::IdComponent visitIndex,
  viskores::IdComponent& contourIndex,
  viskores::Vec2ui_8& edgeBuffer)
{
  if (shape == viskores::CELL_SHAPE_POLYGON)
  {
    viskores::IdComponent numCrossings = 0;
    viskores::IdComponent numIsoValues =
      static_cast<viskores::IdComponent>(isoValues.GetNumberOfValues());
    const viskores::IdComponent numPoints = fieldIn.GetNumberOfComponents();
    for (contourIndex = 0; contourIndex < numIsoValues; ++contourIndex)
    {
      auto isoValue = isoValues.Get(contourIndex);
      bool lastOver = (fieldIn[0] > isoValue);
      for (viskores::IdComponent point = 1; point <= numPoints; ++point)
      {
        bool nextOver = (fieldIn[point % numPoints] > isoValue);
        if (lastOver != nextOver)
        {
          // Check to see if we hit the target edge.
          if (visitIndex == (numCrossings / 2))
          {
            if ((numCrossings % 2) == 0)
            {
              // Record first point.
              edgeBuffer[0] = point - 1;
            }
            else
            {
              // Record second (and final) point.
              edgeBuffer[1] = point - 1;
              return &edgeBuffer[0];
            }
          }
          ++numCrossings;
        }
        lastOver = nextOver;
      }
      VISKORES_ASSERT((numCrossings % 2) == 0);
    }
    VISKORES_ASSERT(0 && "Sanity check fail.");
    edgeBuffer[0] = edgeBuffer[1] = 0;
    return &edgeBuffer[0];
  }
  else
  {
    return TableCellEdges<2>(shape, isoValues, fieldIn, visitIndex, contourIndex);
  }
}

template <typename IsoValuesType, typename FieldVecType>
VISKORES_EXEC const viskores::UInt8* CellEdgesSpecialCases(
  std::integral_constant<viskores::UInt8, 1>,
  viskores::UInt8 shape,
  const IsoValuesType& isoValues,
  const FieldVecType& fieldIn,
  viskores::IdComponent visitIndex,
  viskores::IdComponent& contourIndex,
  viskores::Vec2ui_8& edgeBuffer)
{
  VISKORES_ASSERT((shape == viskores::CELL_SHAPE_LINE) ||
                  (shape == viskores::CELL_SHAPE_POLY_LINE));
  (void)shape;
  viskores::IdComponent numCrossings = 0;
  viskores::IdComponent numIsoValues =
    static_cast<viskores::IdComponent>(isoValues.GetNumberOfValues());
  const viskores::IdComponent numPoints = fieldIn.GetNumberOfComponents();
  for (contourIndex = 0; contourIndex < numIsoValues; ++contourIndex)
  {
    auto isoValue = isoValues.Get(contourIndex);
    bool lastOver = (fieldIn[0] > isoValue);
    for (viskores::IdComponent point = 1; point < numPoints; ++point)
    {
      bool nextOver = (fieldIn[point] > isoValue);
      if (lastOver != nextOver)
      {
        if (visitIndex == numCrossings)
        {
          edgeBuffer[0] = point - 1;
          return &edgeBuffer[0];
        }
        ++numCrossings;
      }
      lastOver = nextOver;
    }
  }
  VISKORES_ASSERT(0 && "Sanity check fail.");
  edgeBuffer[0] = 0;
  return &edgeBuffer[0];
}

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
// -----------------------------------------------------------------------------
template <viskores::UInt8 Dims>
class EdgeWeightGenerate : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ScatterType = viskores::worklet::ScatterCounting;

  template <typename ArrayHandleType>
  VISKORES_CONT static ScatterType MakeScatter(const ArrayHandleType& numOutputTrisPerCell)
  {
    return ScatterType(numOutputTrisPerCell);
  }

  typedef void ControlSignature(CellSetIn cellset, // Cell set
                                WholeArrayIn isoValues,
                                FieldInPoint fieldIn, // Input point field defining the contour
                                ExecObject metaData); // Metadata for edge weight generation
  using ExecutionSignature =
    void(CellShape, PointCount, _2, _3, _4, InputIndex, WorkIndex, VisitIndex, PointIndices);

  using InputDomain = _1;

  template <typename CellShape,
            typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename IndicesVecType>
  VISKORES_EXEC void operator()(
    const CellShape shape,
    viskores::IdComponent numVertices,
    const IsoValuesType& isovalues,
    const FieldInType& fieldIn, // Input point field defining the contour
    const EdgeWeightGenerateMetaData::ExecObject& metaData,
    viskores::Id inputCellId,
    viskores::Id outputCellId,
    viskores::IdComponent visitIndex,
    const IndicesVecType& indices) const
  {
    const viskores::Id outputPointId = OutCellTraits<Dims>::NUM_POINTS * outputCellId;
    using FieldType = typename viskores::VecTraits<FieldInType>::ComponentType;

    // Interpolate for vertex positions and associated scalar values
    viskores::IdComponent contourIndex;
    viskores::Vec2ui_8 edgeBuffer;
    const viskores::UInt8* edges =
      CellEdgesSpecialCases(std::integral_constant<viskores::UInt8, Dims>{},
                            shape.Id,
                            isovalues,
                            fieldIn,
                            visitIndex,
                            contourIndex,
                            edgeBuffer);
    for (viskores::IdComponent triVertex = 0; triVertex < OutCellTraits<Dims>::NUM_POINTS;
         triVertex++)
    {
      viskores::IdComponent2 edgeVertices;
      viskores::Vec<FieldType, 2> fieldValues;
      for (viskores::IdComponent edgePointId = 0; edgePointId < 2; ++edgePointId)
      {
        viskores::ErrorCode errorCode = this->CrossingLocalIndex(
          numVertices, edgePointId, edges[triVertex], shape, edgeVertices[edgePointId]);
        if (errorCode != viskores::ErrorCode::Success)
        {
          this->RaiseError(viskores::ErrorString(errorCode));
          return;
        }
        fieldValues[edgePointId] = fieldIn[edgeVertices[edgePointId]];
      }

      // Store the input cell id so that we can properly generate the normals
      // in a subsequent call, after we have merged duplicate points
      metaData.InterpCellIdPortal.Set(outputPointId + triVertex, inputCellId);

      metaData.InterpContourPortal.Set(outputPointId + triVertex,
                                       static_cast<viskores::UInt8>(contourIndex));

      metaData.InterpIdPortal.Set(
        outputPointId + triVertex,
        viskores::Id2(indices[edgeVertices[0]], indices[edgeVertices[1]]));

      viskores::FloatDefault interpolant =
        static_cast<viskores::FloatDefault>(isovalues.Get(contourIndex) - fieldValues[0]) /
        static_cast<viskores::FloatDefault>(fieldValues[1] - fieldValues[0]);

      metaData.InterpWeightsPortal.Set(outputPointId + triVertex, interpolant);
    }
  }

  template <typename CellShapeTag>
  static inline VISKORES_EXEC viskores::ErrorCode CrossingLocalIndex(
    viskores::IdComponent numPoints,
    viskores::IdComponent pointIndex,
    viskores::IdComponent edgeIndex,
    CellShapeTag shape,
    viskores::IdComponent& result);
};

template <>
template <typename CellShapeTag>
VISKORES_EXEC viskores::ErrorCode EdgeWeightGenerate<1>::CrossingLocalIndex(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::IdComponent edgeIndex,
  CellShapeTag shape,
  viskores::IdComponent& result)
{
  VISKORES_ASSERT((shape.Id == viskores::CELL_SHAPE_LINE) ||
                  (shape.Id == viskores::CELL_SHAPE_POLY_LINE));
  (void)shape;
  if ((pointIndex < 0) || (pointIndex > 1))
  {
    result = -1;
    return viskores::ErrorCode::InvalidPointId;
  }
  if ((edgeIndex < 0) || (edgeIndex >= (numPoints - 1)))
  {
    result = -1;
    return viskores::ErrorCode::InvalidEdgeId;
  }
  result = edgeIndex + pointIndex;
  return viskores::ErrorCode::Success;
}

template <viskores::UInt8 Dims>
template <typename CellShapeTag>
VISKORES_EXEC viskores::ErrorCode EdgeWeightGenerate<Dims>::CrossingLocalIndex(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::IdComponent edgeIndex,
  CellShapeTag shape,
  viskores::IdComponent& result)
{
  return viskores::exec::CellEdgeLocalIndex(numPoints, pointIndex, edgeIndex, shape, result);
}

// ---------------------------------------------------------------------------
struct MultiContourLess
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& a, const T& b) const
  {
    return a < b;
  }

  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const viskores::Pair<T, U>& a,
                                     const viskores::Pair<T, U>& b) const
  {
    return (a.first < b.first) || (!(b.first < a.first) && (a.second < b.second));
  }

  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const viskores::internal::ArrayPortalValueReference<T>& a,
                                     const U& b) const
  {
    U&& t = static_cast<U>(a);
    return t < b;
  }
};

// ---------------------------------------------------------------------------
struct MergeDuplicateValues : viskores::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                ValuesIn valuesIn1,
                                ValuesIn valuesIn2,
                                ReducedValuesOut valueOut1,
                                ReducedValuesOut valueOut2);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename T,
            typename ValuesInType,
            typename Values2InType,
            typename ValuesOutType,
            typename Values2OutType>
  VISKORES_EXEC void operator()(const T&,
                                const ValuesInType& values1,
                                const Values2InType& values2,
                                ValuesOutType& valueOut1,
                                Values2OutType& valueOut2) const
  {
    valueOut1 = values1[0];
    valueOut2 = values2[0];
  }
};

// ---------------------------------------------------------------------------
struct CopyEdgeIds : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VISKORES_EXEC
  void operator()(const viskores::Id2& input, viskores::Id2& output) const { output = input; }

  template <typename T>
  VISKORES_EXEC void operator()(const viskores::Pair<T, viskores::Id2>& input,
                                viskores::Id2& output) const
  {
    output = input.second;
  }
};

// ---------------------------------------------------------------------------
template <typename KeyType, typename KeyStorage>
void MergeDuplicates(const viskores::cont::Invoker& invoker,
                     const viskores::cont::ArrayHandle<KeyType, KeyStorage>& original_keys,
                     viskores::cont::ArrayHandle<viskores::FloatDefault>& weights,
                     viskores::cont::ArrayHandle<viskores::Id2>& edgeIds,
                     viskores::cont::ArrayHandle<viskores::Id>& cellids,
                     viskores::cont::ArrayHandle<viskores::Id>& connectivity)
{
  viskores::cont::ArrayHandle<KeyType> input_keys;
  viskores::cont::ArrayCopyDevice(original_keys, input_keys);
  viskores::worklet::Keys<KeyType> keys(input_keys);
  input_keys.ReleaseResources();

  {
    viskores::cont::ArrayHandle<viskores::Id> writeCells;
    viskores::cont::ArrayHandle<viskores::FloatDefault> writeWeights;
    invoker(MergeDuplicateValues{}, keys, weights, cellids, writeWeights, writeCells);
    weights = writeWeights;
    cellids = writeCells;
  }

  //need to build the new connectivity
  auto uniqueKeys = keys.GetUniqueKeys();
  viskores::cont::Algorithm::LowerBounds(
    uniqueKeys, original_keys, connectivity, marching_cells::MultiContourLess());

  //update the edge ids
  invoker(CopyEdgeIds{}, uniqueKeys, edgeIds);
}

// -----------------------------------------------------------------------------
template <viskores::IdComponent Comp>
struct EdgeVertex
{
  VISKORES_EXEC viskores::Id operator()(const viskores::Id2& edge) const { return edge[Comp]; }
};

class NormalsWorkletPass1 : public viskores::worklet::WorkletVisitPointsWithCells
{
private:
  using PointIdsArray =
    viskores::cont::ArrayHandleTransform<viskores::cont::ArrayHandle<viskores::Id2>, EdgeVertex<0>>;

public:
  using ControlSignature = void(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                FieldOutPoint normals);

  using ExecutionSignature = void(CellCount, CellIndices, InputIndex, _2, _3, _4, _5);

  using InputDomain = _1;
  using ScatterType = viskores::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  VISKORES_CONT
  static ScatterType MakeScatter(const viskores::cont::ArrayHandle<viskores::Id2>& edges)
  {
    return ScatterType(viskores::cont::make_ArrayHandleTransform(edges, EdgeVertex<0>()));
  }

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename NormalType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& numCells,
                                const FromIndexType& cellIds,
                                viskores::Id pointId,
                                const CellSetInType& geometry,
                                const WholeCoordinatesIn& pointCoordinates,
                                const WholeFieldIn& inputField,
                                NormalType& normal) const
  {
    viskores::worklet::gradient::PointGradient gradient;
    gradient(numCells, cellIds, pointId, geometry, pointCoordinates, inputField, normal);
  }

  template <typename FromIndexType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename NormalType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& viskoresNotUsed(numCells),
                                const FromIndexType& viskoresNotUsed(cellIds),
                                viskores::Id pointId,
                                viskores::exec::ConnectivityStructured<Cell, Point, 3>& geometry,
                                const WholeCoordinatesIn& pointCoordinates,
                                const WholeFieldIn& inputField,
                                NormalType& normal) const
  {
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    viskores::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    viskores::exec::arg::ThreadIndicesPointNeighborhood tpn(
      pointId, pointId, 0, pointId, pointGeom);

    const auto& boundary = tpn.GetBoundaryState();
    viskores::exec::FieldNeighborhood<WholeCoordinatesIn> points(pointCoordinates, boundary);
    viskores::exec::FieldNeighborhood<WholeFieldIn> field(inputField, boundary);

    viskores::worklet::gradient::StructuredPointGradient gradient;
    gradient(boundary, points, field, normal);
  }
};

class NormalsWorkletPass2 : public viskores::worklet::WorkletVisitPointsWithCells
{
private:
  using PointIdsArray =
    viskores::cont::ArrayHandleTransform<viskores::cont::ArrayHandle<viskores::Id2>, EdgeVertex<1>>;

public:
  typedef void ControlSignature(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                WholeArrayIn weights,
                                FieldInOutPoint normals);

  using ExecutionSignature =
    void(CellCount, CellIndices, InputIndex, _2, _3, _4, WorkIndex, _5, _6);

  using InputDomain = _1;
  using ScatterType = viskores::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  VISKORES_CONT
  static ScatterType MakeScatter(const viskores::cont::ArrayHandle<viskores::Id2>& edges)
  {
    return ScatterType(viskores::cont::make_ArrayHandleTransform(edges, EdgeVertex<1>()));
  }

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename WholeWeightsIn,
            typename NormalType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& numCells,
                                const FromIndexType& cellIds,
                                viskores::Id pointId,
                                const CellSetInType& geometry,
                                const WholeCoordinatesIn& pointCoordinates,
                                const WholeFieldIn& inputField,
                                viskores::Id edgeId,
                                const WholeWeightsIn& weights,
                                NormalType& normal) const
  {
    viskores::worklet::gradient::PointGradient gradient;
    NormalType grad1;
    gradient(numCells, cellIds, pointId, geometry, pointCoordinates, inputField, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = viskores::Normal(viskores::Lerp(grad0, grad1, weight));
  }

  template <typename FromIndexType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename WholeWeightsIn,
            typename NormalType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& viskoresNotUsed(numCells),
                                const FromIndexType& viskoresNotUsed(cellIds),
                                viskores::Id pointId,
                                viskores::exec::ConnectivityStructured<Cell, Point, 3>& geometry,
                                const WholeCoordinatesIn& pointCoordinates,
                                const WholeFieldIn& inputField,
                                viskores::Id edgeId,
                                const WholeWeightsIn& weights,
                                NormalType& normal) const
  {
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    viskores::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    viskores::exec::arg::ThreadIndicesPointNeighborhood tpn(
      pointId, pointId, 0, pointId, pointGeom);

    const auto& boundary = tpn.GetBoundaryState();
    viskores::exec::FieldNeighborhood<WholeCoordinatesIn> points(pointCoordinates, boundary);
    viskores::exec::FieldNeighborhood<WholeFieldIn> field(inputField, boundary);

    viskores::worklet::gradient::StructuredPointGradient gradient;
    NormalType grad1;
    gradient(boundary, points, field, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = viskores::Lerp(grad0, grad1, weight);
    const auto mag2 = viskores::MagnitudeSquared(normal);
    if (mag2 > 0.)
    {
      normal = normal * viskores::RSqrt(mag2);
    }
  }
};


struct GenerateNormals
{
  template <typename CoordinateSystem,
            typename NormalCType,
            typename InputFieldType,
            typename InputStorageType,
            typename CellSet>
  void operator()(const CoordinateSystem& coordinates,
                  const viskores::cont::Invoker& invoker,
                  viskores::cont::ArrayHandle<viskores::Vec<NormalCType, 3>>& normals,
                  const viskores::cont::ArrayHandle<InputFieldType, InputStorageType>& field,
                  const CellSet cellset,
                  const viskores::cont::ArrayHandle<viskores::Id2>& edges,
                  const viskores::cont::ArrayHandle<viskores::FloatDefault>& weights) const
  {
    // To save memory, the normals computation is done in two passes. In the first
    // pass the gradient at the first vertex of each edge is computed and stored in
    // the normals array. In the second pass the gradient at the second vertex is
    // computed and the gradient of the first vertex is read from the normals array.
    // The final normal is interpolated from the two gradient values and stored
    // in the normals array.
    //
    auto scalarField = marching_cells::make_ScalarField(field);
    invoker(NormalsWorkletPass1{},
            NormalsWorkletPass1::MakeScatter(edges),
            cellset,
            cellset,
            coordinates,
            scalarField,
            normals);

    invoker(NormalsWorkletPass2{},
            NormalsWorkletPass2::MakeScatter(edges),
            cellset,
            cellset,
            coordinates,
            scalarField,
            weights,
            normals);
  }
};

//----------------------------------------------------------------------------
template <viskores::UInt8 Dims,
          typename CellSetType,
          typename CoordinateSystem,
          typename ValueType,
          typename StorageTagField>
viskores::cont::CellSetSingleType<> execute(
  const CellSetType& cells,
  const CoordinateSystem& coordinateSystem,
  const std::vector<ValueType>& isovalues,
  const viskores::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState)
{
  using viskores::worklet::contour::MapPointField;
  using viskores::worklet::marching_cells::ClassifyCell;
  using viskores::worklet::marching_cells::EdgeWeightGenerate;
  using viskores::worklet::marching_cells::EdgeWeightGenerateMetaData;

  // Setup the invoker
  viskores::cont::Invoker invoker;

  viskores::cont::ArrayHandle<ValueType> isoValuesHandle =
    viskores::cont::make_ArrayHandle(isovalues, viskores::CopyFlag::Off);

  // Call the ClassifyCell functor to compute the Marching Cubes case numbers
  // for each cell, and the number of vertices to be generated
  viskores::cont::ArrayHandle<viskores::IdComponent> numOutputTrisPerCell;
  {
    marching_cells::ClassifyCell<Dims, ValueType> classifyCell;
    invoker(classifyCell, isoValuesHandle, inputField, cells, numOutputTrisPerCell);
  }

  //Pass 2 Generate the edges
  viskores::cont::ArrayHandle<viskores::UInt8> contourIds;
  viskores::cont::ArrayHandle<viskores::Id> originalCellIdsForPoints;
  {
    auto scatter = EdgeWeightGenerate<Dims>::MakeScatter(numOutputTrisPerCell);

    // Maps output cells to input cells. Store this for cell field mapping.
    sharedState.CellIdMap = scatter.GetOutputToInputMap();

    EdgeWeightGenerateMetaData metaData(
      Dims,
      scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
      sharedState.InterpolationWeights,
      sharedState.InterpolationEdgeIds,
      originalCellIdsForPoints,
      contourIds);

    invoker(EdgeWeightGenerate<Dims>{},
            scatter,
            cells,
            //cast to a scalar field if not one, as cellderivative only works on those
            isoValuesHandle,
            inputField,
            metaData);
  }

  if (isovalues.size() <= 1 || !sharedState.MergeDuplicatePoints)
  { //release memory early that we are not going to need again
    contourIds.ReleaseResources();
  }

  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  if (sharedState.MergeDuplicatePoints)
  {
    // In all the below cases you will notice that only interpolation ids
    // are updated. That is because MergeDuplicates will internally update
    // the InterpolationWeights and InterpolationOriginCellIds arrays to be the correct for the
    // output. But for InterpolationEdgeIds we need to do it manually once done
    if (isovalues.size() == 1)
    {
      marching_cells::MergeDuplicates(invoker,
                                      sharedState.InterpolationEdgeIds, //keys
                                      sharedState.InterpolationWeights, //values
                                      sharedState.InterpolationEdgeIds, //values
                                      originalCellIdsForPoints,         //values
                                      connectivity); // computed using lower bounds
    }
    else
    {
      marching_cells::MergeDuplicates(
        invoker,
        viskores::cont::make_ArrayHandleZip(contourIds, sharedState.InterpolationEdgeIds), //keys
        sharedState.InterpolationWeights,                                                  //values
        sharedState.InterpolationEdgeIds,                                                  //values
        originalCellIdsForPoints,                                                          //values
        connectivity); // computed using lower bounds
    }
  }
  else
  {
    //when we don't merge points, the connectivity array can be represented
    //by a counting array. The danger of doing it this way is that the output
    //type is unknown. That is why we copy it into an explicit array
    viskores::cont::ArrayHandleIndex temp(sharedState.InterpolationEdgeIds.GetNumberOfValues());
    viskores::cont::ArrayCopy(temp, connectivity);
  }

  //generate the vertices's
  invoker(MapPointField{},
          sharedState.InterpolationEdgeIds,
          sharedState.InterpolationWeights,
          coordinateSystem,
          vertices);

  //assign the connectivity to the cell set
  viskores::cont::CellSetSingleType<> outputCells;
  outputCells.Fill(vertices.GetNumberOfValues(),
                   OutCellTraits<Dims>::CELL_SHAPE,
                   OutCellTraits<Dims>::NUM_POINTS,
                   connectivity);

  //now that the vertices have been generated we can generate the normals
  if (sharedState.GenerateNormals)
  {
    GenerateNormals genNorms;
    genNorms(coordinateSystem,
             invoker,
             normals,
             inputField,
             cells,
             sharedState.InterpolationEdgeIds,
             sharedState.InterpolationWeights);
  }

  return outputCells;
}
}
}
} // namespace viskores::worklet::marching_cells

#endif // viskores_worklet_contour_MarchingCells_h
