// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractCellsAlongPolyLine.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLocator.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractCellsAlongPolyLine);

namespace
{
//==============================================================================
// This struct is a toolset of elements needed by a vtkDataSet to extract the points of a given
// cell.
struct DataSetHelper
{
  using DataSetType = vtkDataSet;

  DataSetHelper(vtkDataSet* input)
    : Input(input)
  {
  }

  vtkDataSet* Input;
  vtkNew<vtkIdList> PointIds;
};

//==============================================================================
// This struct is a toolset of elements needed by a vtkUnstructuredGrid to extract the points
// of a given cell.
template <class ConnectivityArrayT>
struct UnstructuredGridHelper
{
  using ConnectivityArrayType = ConnectivityArrayT;
  using DataSetType = vtkUnstructuredGrid;

  UnstructuredGridHelper(vtkUnstructuredGrid* input)
    : Input(input)
  {
    vtkCellArray* cells = input->GetCells();
    this->Connectivity = vtkArrayDownCast<ConnectivityArrayType>(cells->GetConnectivityArray());
    this->Offsets = vtkArrayDownCast<ConnectivityArrayType>(cells->GetOffsetsArray());
  }

  vtkUnstructuredGrid* Input;
  ConnectivityArrayType* Connectivity;
  ConnectivityArrayType* Offsets;
};

//==============================================================================
// Helper class to read input cells depending on the input's type
// This class implements:
// * AddHitCellIdsAndPointIds: Given an input cell that is hit by an input line,
//   add the cell id in a cell id container and the points of the cell in a point id container,
//   and update the connectivity size of the output unstructured grid
// * CopyCell: Given an input input cell id, copy it into the output unstructured grid.
template <class DataSetT>
struct InputCellHandler;

//==============================================================================
template <>
struct InputCellHandler<vtkDataSet>
{
  static void AddHitCellIdsAndPointIds(vtkIdType cellId, const ::DataSetHelper& helper,
    vtkIdType& connectivitySize, std::unordered_set<vtkIdType>& intersectedCellIds,
    std::unordered_set<vtkIdType>& intersectedCellPointIds);

  template <class ArrayRangeT>
  static void CopyCell(vtkIdType cellId, const ::DataSetHelper& helper,
    const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap,
    vtkIdType currentOffset, ArrayRangeT& outputConnectivity);
};

//==============================================================================
template <>
struct InputCellHandler<vtkUnstructuredGrid>
{
  template <class DataSetHelperT>
  static void AddHitCellIdsAndPointIds(vtkIdType cellId, const DataSetHelperT& helper,
    vtkIdType& connectivitySize, std::unordered_set<vtkIdType>& intersectedCellIds,
    std::unordered_set<vtkIdType>& intersectedCellPointIds);

  template <class DataSetHelperT, class ArrayRangeT>
  static void CopyCell(vtkIdType cellId, const DataSetHelperT& helper,
    const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap,
    vtkIdType currentOffset, ArrayRangeT& outputConnectivity);
};

//------------------------------------------------------------------------------
void InputCellHandler<vtkDataSet>::AddHitCellIdsAndPointIds(vtkIdType cellId,
  const ::DataSetHelper& helper, vtkIdType& connectivitySize,
  std::unordered_set<vtkIdType>& intersectedCellIds,
  std::unordered_set<vtkIdType>& intersectedCellPointIds)
{
  vtkIdList* cellPointIds = helper.PointIds;
  helper.Input->GetCellPoints(cellId, cellPointIds);
  connectivitySize += intersectedCellIds.count(cellId) ? 0 : cellPointIds->GetNumberOfIds();
  intersectedCellIds.insert(cellId);

  for (vtkIdType cellPointId = 0; cellPointId < cellPointIds->GetNumberOfIds(); ++cellPointId)
  {
    intersectedCellPointIds.insert(cellPointIds->GetId(cellPointId));
  }
}

//------------------------------------------------------------------------------
template <class DataSetHelperT>
void InputCellHandler<vtkUnstructuredGrid>::AddHitCellIdsAndPointIds(vtkIdType cellId,
  const DataSetHelperT& helper, vtkIdType& connectivitySize,
  std::unordered_set<vtkIdType>& intersectedCellIds,
  std::unordered_set<vtkIdType>& intersectedCellPointIds)
{
  auto connectivity = vtk::DataArrayValueRange<1>(helper.Connectivity);
  auto offsets = vtk::DataArrayValueRange<1>(helper.Offsets);

  vtkIdType startId = static_cast<vtkIdType>(offsets[cellId]);
  vtkIdType endId = static_cast<vtkIdType>(offsets[cellId + 1]);

  connectivitySize += intersectedCellIds.count(cellId) ? 0 : endId - startId;
  intersectedCellIds.insert(cellId);

  for (vtkIdType id = startId; id < endId; ++id)
  {
    intersectedCellPointIds.insert(connectivity[id]);
  }
}

//------------------------------------------------------------------------------
template <class ArrayRangeT>
void InputCellHandler<vtkDataSet>::CopyCell(vtkIdType inputCellId, const ::DataSetHelper& helper,
  const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap, vtkIdType currentOffset,
  ArrayRangeT& outputConnectivity)
{
  vtkIdList* pointIds = helper.PointIds;
  helper.Input->GetCellPoints(inputCellId, pointIds);

  for (vtkIdType pointId = 0; pointId < pointIds->GetNumberOfIds(); ++pointId)
  {
    outputConnectivity[currentOffset + pointId] =
      inputToOutputPointIdMap.at(pointIds->GetId(pointId));
  }
}

//------------------------------------------------------------------------------
template <class DataSetHelperT, class ArrayRangeT>
void InputCellHandler<vtkUnstructuredGrid>::CopyCell(vtkIdType inputCellId,
  const DataSetHelperT& helper,
  const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap, vtkIdType currentOffset,
  ArrayRangeT& outputConnectivity)
{
  auto inputConnectivity = vtk::DataArrayValueRange<1>(helper.Connectivity);
  auto inputOffsets = vtk::DataArrayValueRange<1>(helper.Offsets);

  vtkIdType startId = static_cast<vtkIdType>(inputOffsets[inputCellId]);
  vtkIdType endId = static_cast<vtkIdType>(inputOffsets[inputCellId + 1]);

  for (vtkIdType id = startId; id < endId; ++id)
  {
    outputConnectivity[currentOffset + id - startId] =
      inputToOutputPointIdMap.at(inputConnectivity[id]);
  }
}

//==============================================================================
template <class DataSetHelperT, class LineCellArrayT>
struct IntersectLinesWorker
{
  using DataSetType = typename DataSetHelperT::DataSetType;

  IntersectLinesWorker(DataSetType* input, vtkCellArray* lineCells,
    vtkUnsignedCharArray* lineCellTypes, vtkPoints* linePoints, vtkAbstractCellLocator* locator,
    vtkIdType& connectivitySize, std::unordered_set<vtkIdType>& intersectedCellIds,
    std::unordered_set<vtkIdType>& intersectedCellPointIds, vtkExtractCellsAlongPolyLine* filter)
    : Input(input)
    , LineCells(lineCells)
    , LineCellTypes(lineCellTypes)
    , LinePoints(linePoints)
    , Locator(locator)
    , GlobalIntersectedCellIds(intersectedCellIds)
    , GlobalIntersectedCellPointIds(intersectedCellPointIds)
    , GlobalConnectivitySize(connectivitySize)
    , Filter(filter)
  {
  }

  void Initialize() { this->ConnectivitySize.Local() = 0; }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    auto lineConnectivity = vtk::DataArrayValueRange<1>(
      vtkArrayDownCast<LineCellArrayT>(this->LineCells->GetConnectivityArray()));
    auto lineOffsets = vtk::DataArrayValueRange<1>(
      vtkArrayDownCast<LineCellArrayT>(this->LineCells->GetOffsetsArray()));

    using ValueType = typename decltype(lineConnectivity)::value_type;

    DataSetHelperT helper(this->Input);
    double p1[3], p2[3];
    vtkNew<vtkIdList> cellIds;

    std::unordered_set<vtkIdType>& intersectedCellIds = this->IntersectedCellIds.Local();
    std::unordered_set<vtkIdType>& intersectedCellPointIds = this->IntersectedCellPointIds.Local();
    vtkIdType& connectivitySize = this->ConnectivitySize.Local();

    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endId - startId) / 10 + 1, (vtkIdType)1000);

    for (vtkIdType lineId = startId; lineId < endId; ++lineId)
    {
      if (lineId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }

        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }

      ValueType start = lineOffsets[lineId];
      ValueType size = lineOffsets[lineId + 1] - start;

      if (this->LineCellTypes)
      {
        unsigned char cellType = this->LineCellTypes->GetValue(lineId);

        // We skip cells that are not lines
        if (cellType != VTK_LINE && cellType != VTK_POLY_LINE)
        {
          vtkLog(WARNING,
            "Cell at id " << lineId
                          << " in the source is not a vtkLine or a vtkPolyLine... Skipping.");
          continue;
        }
      }

      for (ValueType pointId = 0; pointId < size - 1; ++pointId)
      {
        this->LinePoints->GetPoint(lineConnectivity[start + pointId], p1);
        this->LinePoints->GetPoint(lineConnectivity[start + pointId + 1], p2);

        // tolerance is ignored in vtkStaticCellLocator
        this->Locator->FindCellsAlongLine(p1, p2, 0.0 /* tolerance */, cellIds);

        for (vtkIdType id = 0; id < cellIds->GetNumberOfIds(); ++id)
        {
          vtkIdType cellId = cellIds->GetId(id);
          InputCellHandler<DataSetType>::AddHitCellIdsAndPointIds(
            cellId, helper, connectivitySize, intersectedCellIds, intersectedCellPointIds);
        }
      }
    }
  }

  void Reduce()
  {
    for (const std::unordered_set<vtkIdType>& ids : this->IntersectedCellIds)
    {
      for (const vtkIdType& id : ids)
      {
        this->GlobalIntersectedCellIds.insert(id);
      }
    }
    for (const std::unordered_set<vtkIdType>& ids : this->IntersectedCellPointIds)
    {
      for (const vtkIdType& id : ids)
      {
        this->GlobalIntersectedCellPointIds.insert(id);
      }
    }
    for (vtkIdType size : this->ConnectivitySize)
    {
      this->GlobalConnectivitySize += size;
    }
  }

  DataSetType* Input;
  vtkCellArray* LineCells;
  vtkUnsignedCharArray* LineCellTypes;
  vtkPoints* LinePoints;
  vtkAbstractCellLocator* Locator;

  std::unordered_set<vtkIdType>& GlobalIntersectedCellIds;
  std::unordered_set<vtkIdType>& GlobalIntersectedCellPointIds;
  vtkIdType& GlobalConnectivitySize;
  vtkExtractCellsAlongPolyLine* Filter;

  vtkSMPThreadLocal<std::unordered_set<vtkIdType>> IntersectedCellIds;
  vtkSMPThreadLocal<std::unordered_set<vtkIdType>> IntersectedCellPointIds;
  vtkSMPThreadLocal<vtkIdType> ConnectivitySize;
};

//------------------------------------------------------------------------------
template <class DataSetHelperT, class LineCellArrayT>
void IntersectLines(typename DataSetHelperT::DataSetType* input, vtkCellArray* lineCells,
  vtkUnsignedCharArray* lineCellTypes, vtkPoints* linePoints, vtkAbstractCellLocator* locator,
  vtkIdType& connectivitySize, std::unordered_set<vtkIdType>& intersectedCellIds,
  std::unordered_set<vtkIdType>& intersectedCellPointIds, vtkExtractCellsAlongPolyLine* self)
{
  IntersectLinesWorker<DataSetHelperT, LineCellArrayT> worker(input, lineCells, lineCellTypes,
    linePoints, locator, connectivitySize, intersectedCellIds, intersectedCellPointIds, self);

  vtkSMPTools::For(0, lineCells->GetNumberOfCells(), worker);
}

//==============================================================================
template <class LineCellArrayT>
struct InputUnstructuredGridCellArrayDispatcher
{
  template <class InputCellArrayT>
  void operator()(InputCellArrayT*, vtkUnstructuredGrid* input, vtkCellArray* lineCells,
    vtkUnsignedCharArray* lineCellTypes, vtkPoints* linePoints, vtkAbstractCellLocator* locator,
    vtkIdType& connectivitySize, std::unordered_set<vtkIdType>& intersectedCellIds,
    std::unordered_set<vtkIdType>& intersectedCellPointIds, vtkExtractCellsAlongPolyLine* self)
  {
    ::IntersectLines<::UnstructuredGridHelper<InputCellArrayT>, LineCellArrayT>(input, lineCells,
      lineCellTypes, linePoints, locator, connectivitySize, intersectedCellIds,
      intersectedCellPointIds, self);
  }
};

//==============================================================================
struct DataSetPointsCopyWorker
{
  DataSetPointsCopyWorker(vtkDataSet* input, vtkPoints* outputPoints, vtkIdList* pointIds,
    vtkExtractCellsAlongPolyLine* filter)
    : Input(input)
    , OutputPoints(outputPoints)
    , PointIds(pointIds)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    double p[3];
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endId - startId) / 10 + 1, (vtkIdType)1000);

    for (vtkIdType pointId = startId; pointId < endId; ++pointId)
    {
      if (pointId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }

        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }

      this->Input->GetPoint(this->PointIds->GetId(pointId), p);
      this->OutputPoints->SetPoint(pointId, p);
    }
  }

  vtkDataSet* Input;
  vtkPoints* OutputPoints;
  vtkIdList* PointIds;
  vtkExtractCellsAlongPolyLine* Filter;
};

//==============================================================================
struct PointSetPointsCopyDispatcher
{
  vtkExtractCellsAlongPolyLine* Filter;

  PointSetPointsCopyDispatcher(vtkExtractCellsAlongPolyLine* filter)
    : Filter(filter)
  {
  }
  template <class ArrayT1, class ArrayT2>
  void operator()(
    ArrayT1* source, ArrayT2* dest, vtkIdList* ids, vtkIdType startId, vtkIdType endId)
  {
    auto sourceRange = vtk::DataArrayTupleRange<3>(source);
    auto destRange = vtk::DataArrayTupleRange<3>(dest);
    using ConstSourceReference = typename decltype(sourceRange)::ConstTupleReferenceType;
    using DestReference = typename decltype(destRange)::TupleReferenceType;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endId - startId) / 10 + 1, (vtkIdType)1000);

    for (vtkIdType pointId = startId; pointId < endId; ++pointId)
    {
      if (pointId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }

        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      ConstSourceReference sourceTuple = sourceRange[ids->GetId(pointId)];
      DestReference destTuple = destRange[pointId];
      std::copy(sourceTuple.begin(), sourceTuple.end(), destTuple.begin());
    }
  }
};

//==============================================================================
struct PointSetPointsCopyWorker
{
  PointSetPointsCopyWorker(
    vtkPoints* input, vtkPoints* output, vtkIdList* pointIds, vtkExtractCellsAlongPolyLine* filter)
    : Input(input)
    , Output(output)
    , PointIds(pointIds)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    using FloatTypes = vtkTypeList::Create<float, double>;
    using Dispatch = vtkArrayDispatch::Dispatch2BySameValueType<FloatTypes>;

    ::PointSetPointsCopyDispatcher dispatcher(this->Filter);
    vtkDataArray* inputData = this->Input->GetData();
    vtkDataArray* outputData = this->Output->GetData();

    if (!Dispatch::Execute(inputData, outputData, dispatcher, this->PointIds, startId, endId))
    {
      // fallback if dispatching fails
      dispatcher(inputData, outputData, this->PointIds, startId, endId);
    }
  }

  vtkPoints* Input;
  vtkPoints* Output;
  vtkIdList* PointIds;
  vtkExtractCellsAlongPolyLine* Filter;
};

//==============================================================================
template <class DataSetHelperT, class OutputCellArrayRangeT>
struct GenerateOutputCellsWorker
{
  using DataSetType = typename DataSetHelperT::DataSetType;

  GenerateOutputCellsWorker(vtkIdList* cellIds, DataSetType* input,
    const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap,
    OutputCellArrayRangeT& outputConnectivity, OutputCellArrayRangeT& outputOffsets,
    vtkUnsignedCharArray* outputCellTypes, vtkExtractCellsAlongPolyLine* filter)
    : CellIds(cellIds)
    , Input(input)
    , InputToOutputPointIdMap(inputToOutputPointIdMap)
    , OutputConnectivity(outputConnectivity)
    , OutputOffsets(outputOffsets)
    , OutputCellTypes(outputCellTypes)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    DataSetHelperT helper(this->Input);
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endId - startId) / 10 + 1, (vtkIdType)1000);

    for (vtkIdType outputCellId = startId; outputCellId < endId; ++outputCellId)
    {
      if (outputCellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }

        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      vtkIdType inputCellId = this->CellIds->GetId(outputCellId);
      vtkIdType currentOffset = this->OutputOffsets[outputCellId];

      ::InputCellHandler<DataSetType>::CopyCell(inputCellId, helper, this->InputToOutputPointIdMap,
        currentOffset, this->OutputConnectivity);

      this->OutputCellTypes->SetValue(outputCellId, this->Input->GetCellType(inputCellId));
    }
  }

  vtkIdList* CellIds;
  DataSetType* Input;
  const std::unordered_map<vtkIdType, vtkIdType>& InputToOutputPointIdMap;
  OutputCellArrayRangeT& OutputConnectivity;
  OutputCellArrayRangeT& OutputOffsets;
  vtkUnsignedCharArray* OutputCellTypes;
  vtkExtractCellsAlongPolyLine* Filter;
};

//------------------------------------------------------------------------------
template <class DataSetHelperT, class OutputArrayT>
void GenerateOutputCells(vtkIdList* cellIds,
  const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap,
  typename DataSetHelperT::DataSetType* input, vtkIdType connectivitySize,
  vtkCellArray* outputCells, vtkUnsignedCharArray* outputCellTypes,
  vtkExtractCellsAlongPolyLine* filter)
{
  vtkIdType numberOfOutputCells = cellIds->GetNumberOfIds();
  DataSetHelperT helper(input);

  vtkDataArray* outputConnectivityDataArray = outputCells->GetConnectivityArray();
  vtkDataArray* outputOffsetsDataArray = outputCells->GetOffsetsArray();

  outputConnectivityDataArray->SetNumberOfTuples(connectivitySize);
  outputOffsetsDataArray->SetNumberOfTuples(numberOfOutputCells + 1);

  auto outputConnectivity =
    vtk::DataArrayValueRange<1>(vtkArrayDownCast<OutputArrayT>(outputConnectivityDataArray));
  auto outputOffsets =
    vtk::DataArrayValueRange<1>(vtkArrayDownCast<OutputArrayT>(outputOffsetsDataArray));
  outputOffsets[0] = 0;

  outputCellTypes->SetNumberOfValues(numberOfOutputCells);
  vtkIdType currentOffset = 0;

  for (vtkIdType outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
  {
    currentOffset += input->GetCellSize(cellIds->GetId(outputCellId));
    outputOffsets[outputCellId + 1] = currentOffset;
  }

  ::GenerateOutputCellsWorker<DataSetHelperT, decltype(outputConnectivity)> worker(cellIds, input,
    inputToOutputPointIdMap, outputConnectivity, outputOffsets, outputCellTypes, filter);

  vtkSMPTools::For(0, numberOfOutputCells, worker);
}

//==============================================================================
struct GenerateOutputCellsWithInputUnstructuredGridDispatcher
{
  template <class InputArrayT, class OutputArrayT>
  void operator()(InputArrayT*, OutputArrayT*, vtkIdList* cellIds,
    const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap,
    vtkUnstructuredGrid* input, vtkIdType connectivitySize, vtkCellArray* outputCells,
    vtkUnsignedCharArray* outputCellTypes, vtkExtractCellsAlongPolyLine* filter)
  {
    ::GenerateOutputCells<::UnstructuredGridHelper<InputArrayT>, OutputArrayT>(cellIds,
      inputToOutputPointIdMap, input, connectivitySize, outputCells, outputCellTypes, filter);
  }
};

//==============================================================================
struct GenerateOutputCellsWithInputDataSetDispatcher
{
  template <class OutputArrayT>
  void operator()(OutputArrayT*, vtkIdList* cellIds,
    const std::unordered_map<vtkIdType, vtkIdType>& inputToOutputPointIdMap, vtkDataSet* input,
    vtkIdType connectivitySize, vtkCellArray* outputCells, vtkUnsignedCharArray* outputCellTypes,
    vtkExtractCellsAlongPolyLine* filter)
  {
    ::GenerateOutputCells<::DataSetHelper, OutputArrayT>(cellIds, inputToOutputPointIdMap, input,
      connectivitySize, outputCells, outputCellTypes, filter);
  }
};

//------------------------------------------------------------------------------
// This function extracts the cells in the input that are intersected by a set of input lines.
// The algorithm is as follows:
// * Intersect the lines using a cell locator and store their cell ids and point ids belonging to
//   each hit cell inside a std::unordered_set (for unicity).
// * Copy each unordered_set into a vtkIdList* and sort the ids to preserve the same ordering as in
//   the input.
//   Note: a std::unoredered_set is used as it is much faster than a std::set, even when calling
//   std::sort later on. Moreover, the more entropy there is in a container,
//   the faster std::sort is likely to be.
// * Copy input cells and points into output
template <class LineCellArrayT>
int ExtractCells(vtkExtractCellsAlongPolyLine* self, vtkDataSet* input, vtkPointSet* lines,
  vtkUnstructuredGrid* output)
{
  using CellArrayTypes = vtkCellArray::StorageArrayList;

  vtkNew<vtkStaticCellLocator> locator;
  locator->SetDataSet(input);
  locator->BuildLocator();

  auto linesUG = vtkUnstructuredGrid::SafeDownCast(lines);
  auto linesPD = vtkPolyData::SafeDownCast(lines);

  if (!linesUG && !linesPD)
  {
    vtkLog(ERROR, "Input lines should be vtkUnstructuredGrid or vtkPolyData");
    return 0;
  }

  vtkPoints* linePoints = lines->GetPoints();

  std::unordered_set<vtkIdType> intersectedCellIds, intersectedCellPointIds;
  vtkIdType connectivitySize = 0;

  // This filter supports lines inside vtkPolyData as well as vtkUnstructuredGrid
  vtkCellArray* lineCells = linesPD ? linesPD->GetLines() : linesUG->GetCells();

  vtkUnsignedCharArray* lineCellTypes = linesUG ? linesUG->GetCellTypesArray() : nullptr;

  auto inputUG = vtkUnstructuredGrid::SafeDownCast(input);

  // We take a fast path when input is an unstructured grid, as we can read the cell arrays directly
  // on intersected cells.
  if (inputUG)
  {
    vtkCellArray* cells = inputUG->GetCells();
    if (cells && cells->GetNumberOfCells())
    {
      ::InputUnstructuredGridCellArrayDispatcher<LineCellArrayT> dispatcher;
      if (!vtkArrayDispatch::DispatchByArray<CellArrayTypes>::Execute(cells->GetConnectivityArray(),
            dispatcher, inputUG, lineCells, lineCellTypes, linePoints, locator, connectivitySize,
            intersectedCellIds, intersectedCellPointIds, self))
      {
        // fallback if dispatching fails
        dispatcher(cells->GetConnectivityArray(), inputUG, lineCells, lineCellTypes, linePoints,
          locator, connectivitySize, intersectedCellIds, intersectedCellPointIds, self);
      }
    }
    else
    {
      // There are no cells to intersect.
      return 1;
    }
  }
  else
  {
    ::IntersectLines<::DataSetHelper, LineCellArrayT>(input, lineCells, lineCellTypes, linePoints,
      locator, connectivitySize, intersectedCellIds, intersectedCellPointIds, self);
  }

  // Sorting cell ids and point ids
  vtkNew<vtkIdList> sortedIntersectedCellIds;
  sortedIntersectedCellIds->SetNumberOfIds(intersectedCellIds.size());
  std::copy(
    intersectedCellIds.cbegin(), intersectedCellIds.cend(), sortedIntersectedCellIds->begin());
  std::sort(sortedIntersectedCellIds->begin(), sortedIntersectedCellIds->end());

  vtkNew<vtkIdList> sortedIntersectedCellPointIds;
  sortedIntersectedCellPointIds->SetNumberOfIds(intersectedCellPointIds.size());
  std::copy(intersectedCellPointIds.cbegin(), intersectedCellPointIds.cend(),
    sortedIntersectedCellPointIds->begin());
  std::sort(sortedIntersectedCellPointIds->begin(), sortedIntersectedCellPointIds->end());

  std::unordered_map<vtkIdType, vtkIdType> inputToOutputPointIdMap;

  vtkIdType numberOfPoints = sortedIntersectedCellPointIds->GetNumberOfIds();
  vtkIdType numberOfCells = sortedIntersectedCellIds->GetNumberOfIds();

  // Mapping input cell ids to output cell ids
  for (vtkIdType pointId = 0; pointId < numberOfPoints; ++pointId)
  {
    inputToOutputPointIdMap.insert({ sortedIntersectedCellPointIds->GetId(pointId), pointId });
  }

  auto inputPS = vtkPointSet::SafeDownCast(input);
  vtkPoints* inputPoints = inputPS ? inputPS->GetPoints() : nullptr;

  // Handling of output points precision
  vtkNew<vtkPoints> points;
  if (inputPoints)
  {
    points->SetDataType(inputPoints->GetDataType());
  }
  else
  {
    switch (self->GetOutputPointsPrecision())
    {
      case vtkAlgorithm::DEFAULT_PRECISION:
      case vtkAlgorithm::SINGLE_PRECISION:
        points->SetDataType(VTK_FLOAT);
        break;
      case vtkAlgorithm::DOUBLE_PRECISION:
        points->SetDataType(VTK_DOUBLE);
        break;
      default:
        vtkLog(WARNING,
          "OutputPointsPrecision is not set to vtkAlgorithm::SINGLE_PRECISION"
            << " or vtkAlgorithn::DOUBLE_PRECISION");
        points->SetDataType(VTK_FLOAT);
        break;
    }
  }

  points->SetNumberOfPoints(numberOfPoints);
  output->SetPoints(points);

  if (inputPoints)
  {
    ::PointSetPointsCopyWorker worker(inputPoints, points, sortedIntersectedCellPointIds, self);
    vtkSMPTools::For(0, numberOfPoints, worker);
  }
  else
  {
    ::DataSetPointsCopyWorker worker(input, points, sortedIntersectedCellPointIds, self);
    vtkSMPTools::For(0, numberOfPoints, worker);
  }

  vtkNew<vtkCellArray> outputCells;
  vtkNew<vtkUnsignedCharArray> outputCellTypes;

#ifdef VTK_USE_64BIT_IDS
  if (!(numberOfPoints >> 31))
  {
    outputCells->ConvertTo32BitStorage();
  }
#endif

  // Copying input cells into output, compressing cell arrays if possible using 32 bits
  // Fast path is used if input is unstructured grid
  if (inputUG)
  {
    vtkCellArray* cells = inputUG->GetCells();
    if (cells && cells->GetNumberOfCells())
    {
      using Dispatcher = vtkArrayDispatch::Dispatch2ByArray<CellArrayTypes, CellArrayTypes>;
      ::GenerateOutputCellsWithInputUnstructuredGridDispatcher dispatcher;

      if (!Dispatcher::Execute(cells->GetConnectivityArray(), outputCells->GetConnectivityArray(),
            dispatcher, sortedIntersectedCellIds, inputToOutputPointIdMap, inputUG,
            connectivitySize, outputCells, outputCellTypes, self))
      {
        // fallback if dispatching fails
        dispatcher(cells->GetConnectivityArray(), outputCells->GetConnectivityArray(),
          sortedIntersectedCellIds, inputToOutputPointIdMap, inputUG, connectivitySize, outputCells,
          outputCellTypes, self);
      }
    }
    else
    {
      // There are no cells to intersect.
      return 1;
    }
  }
  else
  {
    using Dispatcher = vtkArrayDispatch::DispatchByArray<CellArrayTypes>;
    ::GenerateOutputCellsWithInputDataSetDispatcher dispatcher;

    if (!Dispatcher::Execute(outputCells->GetConnectivityArray(), dispatcher,
          sortedIntersectedCellIds, inputToOutputPointIdMap, input, connectivitySize, outputCells,
          outputCellTypes, self))
    {
      // fallback if dispatching fails
      dispatcher(outputCells->GetConnectivityArray(), sortedIntersectedCellIds,
        inputToOutputPointIdMap, input, connectivitySize, outputCells, outputCellTypes, self);
    }
  }

  output->SetCells(outputCellTypes, outputCells);

  // Copying point and cell data
  vtkCellData* inputCD = input->GetCellData();
  vtkCellData* outputCD = output->GetCellData();

  outputCD->CopyAllOn();
  outputCD->CopyAllocate(inputCD);
  outputCD->SetNumberOfTuples(numberOfCells);
  outputCD->CopyData(inputCD, sortedIntersectedCellIds);

  vtkPointData* inputPD = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();

  outputPD->CopyAllOn();
  outputPD->CopyAllocate(inputPD, numberOfPoints);
  outputPD->SetNumberOfTuples(numberOfPoints);
  outputPD->CopyData(inputPD, sortedIntersectedCellPointIds);

  return 1;
}

//==============================================================================
struct ExtractCellsDispatcher
{
  template <class ArrayT>
  void operator()(ArrayT*, vtkExtractCellsAlongPolyLine* self, vtkDataSet* input,
    vtkPointSet* lines, vtkUnstructuredGrid* output)
  {
    this->ReturnState = ::ExtractCells<ArrayT>(self, input, lines, output);
  }

  int ReturnState = 0;
};
} // anonymous namespace

//------------------------------------------------------------------------------
vtkExtractCellsAlongPolyLine::vtkExtractCellsAlongPolyLine()
  : OutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION)
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkExtractCellsAlongPolyLine::~vtkExtractCellsAlongPolyLine() = default;

//------------------------------------------------------------------------------
int vtkExtractCellsAlongPolyLine::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Check inputs / outputs
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* samplerInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!outInfo || !inputInfo || !samplerInfo)
  {
    vtkErrorMacro("Missing input or output information");
    return 0;
  }

  auto input = vtkDataSet::SafeDownCast(inputInfo->Get(vtkDataObject::DATA_OBJECT()));
  auto linesPS = vtkPointSet::SafeDownCast(samplerInfo->Get(vtkDataObject::DATA_OBJECT()));
  auto output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input || !linesPS)
  {
    vtkErrorMacro("Missing input or output");
    return 0;
  }

  vtkCellArray* cells;
  if (auto linesPD = vtkPolyData::SafeDownCast(linesPS))
  {
    cells = linesPD->GetLines();
  }
  else if (auto linesUG = vtkUnstructuredGrid::SafeDownCast(linesPS))
  {
    cells = linesUG->GetCells();
  }
  else
  {
    vtkErrorMacro("Unsupported source of type "
      << linesPS->GetClassName() << ". It should be a vtkPolyData or a vtkUnstructuredGrid.");
    return 0;
  }

  if (cells && cells->GetNumberOfCells())
  {
    using Dispatcher = vtkArrayDispatch::DispatchByArray<vtkCellArray::StorageArrayList>;
    ::ExtractCellsDispatcher dispatcher;

    if (!Dispatcher::Execute(
          cells->GetConnectivityArray(), dispatcher, this, input, linesPS, output))
    {
      // fallback if dispatch fails
      dispatcher(cells->GetConnectivityArray(), this, input, linesPS, output);
    }
    return dispatcher.ReturnState;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractCellsAlongPolyLine::FillInputPortInformation(int port, vtkInformation* info)
{

  switch (port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      break;

    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
      break;

    default:
      break;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractCellsAlongPolyLine::SetSourceConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}

//------------------------------------------------------------------------------
void vtkExtractCellsAlongPolyLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
