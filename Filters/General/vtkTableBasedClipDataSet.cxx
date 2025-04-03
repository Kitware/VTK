// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTableBasedClipDataSet.h"

#include "vtkAppendFilter.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkBatch.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkClipDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkExtractCells.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataToUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStructuredGrid.h"
#include "vtkTableBasedClipCases.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cstring>
#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkTableBasedClipDataSet);
vtkCxxSetObjectMacro(vtkTableBasedClipDataSet, ClipFunction, vtkImplicitFunction);

//------------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
vtkTableBasedClipDataSet::vtkTableBasedClipDataSet(vtkImplicitFunction* cf)
{
  this->ClipFunction = cf;

  // setup a callback to report progress
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(
    &vtkTableBasedClipDataSet::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->Value = 0.0;
  this->InsideOut = 0;
  this->MergeTolerance = 0.01;
  this->UseValueAsOffset = true;
  this->GenerateClipScalars = 0;
  this->GenerateClippedOutput = 0;

  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->BatchSize = 1000;

  this->SetNumberOfOutputPorts(2);
  vtkNew<vtkUnstructuredGrid> output2;
  this->GetExecutive()->SetOutputData(1, output2);

  // process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkTableBasedClipDataSet::~vtkTableBasedClipDataSet()
{
  this->SetClipFunction(nullptr);
  this->InternalProgressObserver->Delete();
  this->InternalProgressObserver = nullptr;
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::InternalProgressCallbackFunction(
  vtkObject* arg, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkTableBasedClipDataSet*>(clientdata)
    ->InternalProgressCallback(static_cast<vtkAlgorithm*>(arg));
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::InternalProgressCallback(vtkAlgorithm* algorithm)
{
  double progress = algorithm->GetProgress();
  this->UpdateProgress(progress);
  this->CheckAbort();
  if (this->GetAbortOutput())
  {
    algorithm->SetAbortExecuteAndUpdateTime();
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkTableBasedClipDataSet::GetMTime()
{
  vtkMTimeType time;
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

vtkUnstructuredGrid* vtkTableBasedClipDataSet::GetClippedOutput()
{
  if (!this->GenerateClippedOutput)
  {
    return nullptr;
  }

  return vtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//------------------------------------------------------------------------------
int vtkTableBasedClipDataSet::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
//------------------------------------------------------------------------------
int vtkTableBasedClipDataSet::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // input and output information objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input of which we have to create a copy since the clipper requires
  // that InterpolateAllocate() be invoked for the output based on its input in
  // terms of the point data. If the input and output arrays are different,
  // vtkCell3D's Clip will fail. The last argument of InterpolateAllocate makes
  // sure that arrays are shallow-copied from input to inputCopy.
  auto input = vtkDataSet::GetData(inInfo);
  vtkSmartPointer<vtkDataSet> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->CopyStructure(input);
  inputCopy->GetCellData()->PassData(input->GetCellData());
  inputCopy->GetFieldData()->PassData(input->GetFieldData());
  inputCopy->GetPointData()->InterpolateAllocate(input->GetPointData(), 0, 0, 1);

  // get the output (the remaining and the clipped parts)
  auto outputUG = vtkUnstructuredGrid::GetData(outInfo);
  vtkUnstructuredGrid* clippedOutputUG = this->GetClippedOutput();

  vtkDebugMacro(<< "Clipping dataset" << endl);

  const vtkIdType numPoints = inputCopy->GetNumberOfPoints();

  // handling exceptions
  if (numPoints < 1)
  {
    vtkDebugMacro(<< "No data to clip" << endl);
    outputUG = nullptr;
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    vtkErrorMacro(<< "Cannot generate clip scalars "
                  << "if no clip function defined" << endl);
    outputUG = nullptr;
    return 1;
  }

  // check whether the cells are clipped with input scalars or a clip function
  vtkSmartPointer<vtkDoubleArray> scalars;
  if (!this->ClipFunction)
  {
    auto inputArray = this->GetInputArrayToProcess(0, inputVector);
    if (!inputArray)
    {
      vtkErrorMacro(<< "no input scalars." << endl);
      return 1;
    }
    // This is needed by vtkClipDataSet in case we fall back to it.
    inputCopy->GetPointData()->AddArray(inputArray);
    inputCopy->GetPointData()->SetActiveScalars(inputArray->GetName());
    // We (shallow/deep)copy the input scalars into a double array.
    // This is needed to GREATLY minimize compilation time,
    // and avoid using the vtkDataArray API
    scalars = vtkSmartPointer<vtkDoubleArray>::New();
    if (inputArray->GetNumberOfComponents() == 1)
    {
      if (inputArray->GetDataType() == scalars->GetDataType() &&
        inputArray->GetArrayType() == scalars->GetArrayType())
      {
        scalars->ShallowCopy(inputArray);
      }
      else
      {
        scalars->DeepCopy(inputArray);
      }
    }
    else
    {
      scalars->SetNumberOfValues(numPoints);
      vtkSMPTools::For(0, numPoints,
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType i = begin; i < end; i++)
          {
            scalars->SetValue(i, inputArray->GetComponent(i, 0));
          }
        });
    }
  }

  double isoValue = (!this->ClipFunction || this->UseValueAsOffset) ? this->Value : 0.0;
  if (auto imageData = vtkImageData::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(imageData, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(imageData, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto polyData = vtkPolyData::SafeDownCast(inputCopy))
  {
    this->ClipPolyData(polyData, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipPolyData(polyData, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto recGrid = vtkRectilinearGrid::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(recGrid, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(recGrid, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto structGrid = vtkStructuredGrid::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(structGrid, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(structGrid, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto uGrid = vtkUnstructuredGrid::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(uGrid, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(uGrid, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto uGridBase = vtkUnstructuredGridBase::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(uGridBase, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(uGridBase, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto pointset = vtkPointSet::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(pointset, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(pointset, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else
  {
    this->ClipDataSet(inputCopy, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipDataSet(inputCopy, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }

  outputUG->Squeeze();
  outputUG->GetFieldData()->PassData(inputCopy->GetFieldData());

  if (clippedOutputUG)
  {
    clippedOutputUG->Squeeze();
    clippedOutputUG->GetFieldData()->PassData(inputCopy->GetFieldData());
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  this->Locator = locator;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipDataSet(vtkDataSet* pDataSet, vtkUnstructuredGrid* outputUG)
{
  vtkNew<vtkClipDataSet> clipData;
  clipData->SetInputData(pDataSet);
  clipData->SetValue(this->Value);
  clipData->SetInsideOut(this->InsideOut);
  clipData->SetClipFunction(this->ClipFunction);
  clipData->SetUseValueAsOffset(this->UseValueAsOffset);
  clipData->SetGenerateClipScalars(this->GenerateClipScalars);
  clipData->SetContainerAlgorithm(this);
  clipData->SetLocator(this->Locator);
  clipData->SetMergeTolerance(this->MergeTolerance);
  clipData->Update();
  outputUG->ShallowCopy(clipData->GetOutput());
}

namespace // begin anonymous namespace
{
//------------------------------------------------------------------------------
// Extract the clipped cells is a 4-step process
// 1) Determine which input points will be kept using scalars and calculate
//    pointBatches, numberOfKeptPoints, and pointsMap.
//    1) If an implicit function is provided instead of scalars,
//       then the scalars need to be evaluated first.
// 2) Evaluate the input cells and calculate connectivitySize, numberOfOutputCells
//    numberOfCentroids, cellBatches, cellsCase, edges.
// 3) Extract cells and calculate centroids, types, cell array, cell data.
// 4) Extract points and point data.

//-----------------------------------------------------------------------------
// Keep track of output information within each batch of points - this
// information is eventually rolled up into offsets iso that separate threads
// know where to write their data. We need to know how many total points are kept.
struct TableBasedPointBatchData
{
  // In EvaluatePoints::operator() this is used as an accumulator
  // in EvaluatePoints::Reduce() this is changed to an offset
  // This is done to reduce memory footprint.
  vtkIdType PointsOffset;

  TableBasedPointBatchData()
    : PointsOffset(0)
  {
  }
  ~TableBasedPointBatchData() = default;
  TableBasedPointBatchData& operator+=(const TableBasedPointBatchData& other)
  {
    this->PointsOffset += other.PointsOffset;
    return *this;
  }
  TableBasedPointBatchData operator+(const TableBasedPointBatchData& other) const
  {
    TableBasedPointBatchData result = *this;
    result += other;
    return result;
  }
};
using TableBasedPointBatch = vtkBatch<TableBasedPointBatchData>;
using TableBasedPointBatches = vtkBatches<TableBasedPointBatchData>;

//-----------------------------------------------------------------------------
// Determine which input points will be kept using scalars, calculate
// pointBatchInfo, numberOfKeptPoints, and pointsMap.
template <typename TInputIdType, bool TInsideOut>
struct EvaluatePoints
{
  vtkDoubleArray* Scalars;
  double IsoValue;
  vtkIdType NumberOfInputPoints;
  vtkTableBasedClipDataSet* Filter;

  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> PointsMap;

  static constexpr TInputIdType InsideOutValues[2] = { TInsideOut ? 1 : -1, TInsideOut ? -1 : 1 };

  TableBasedPointBatches PointBatches;
  TInputIdType NumberOfKeptPoints;

  EvaluatePoints(vtkDoubleArray* scalars, double isoValue, unsigned int batchSize,
    vtkTableBasedClipDataSet* filter)
    : Scalars(scalars)
    , IsoValue(isoValue)
    , NumberOfInputPoints(scalars->GetNumberOfTuples())
    , Filter(filter)
  {
    // initialize batches
    this->PointBatches.Initialize(this->NumberOfInputPoints, batchSize);

    this->PointsMap = vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>>::New();
    this->PointsMap->SetNumberOfValues(this->NumberOfInputPoints);
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    const auto& scalars = vtk::DataArrayValueRange<1>(this->Scalars);
    auto pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap);
    vtkIdType pointId;
    double grdDiff;

    const bool isFirst = vtkSMPTools::GetSingleThread();
    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      TableBasedPointBatch& batch = this->PointBatches[batchId];
      auto& batchNumberOfPoints = batch.Data.PointsOffset;
      for (pointId = batch.BeginId; pointId < batch.EndId; ++pointId)
      {
        // Outside points are marked with -1, others with 1
        grdDiff = scalars[pointId] - this->IsoValue;
        pointsMap[pointId] = EvaluatePoints::InsideOutValues[grdDiff >= 0.0];
        batchNumberOfPoints += (pointsMap[pointId] > 0);
      }
    }
  }

  void Reduce()
  {
    // trim batches with 0 points in-place
    this->PointBatches.TrimBatches(
      [](const TableBasedPointBatch& batch) { return batch.Data.PointsOffset == 0; });

    // assign beginPointsOffset for each batch
    const auto globalSum = this->PointBatches.BuildOffsetsAndGetGlobalSum();
    this->NumberOfKeptPoints = globalSum.PointsOffset;

    // Prefix sum to create point map of kept (i.e., retained) points.
    auto pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap);
    vtkSMPTools::For(0, this->PointBatches.GetNumberOfBatches(),
      [&](vtkIdType beginBatchId, vtkIdType endBatchId)
      {
        vtkIdType pointId;
        TInputIdType pointsMapValues[2] = { -1 /*always the same*/, 0 /*offset*/ };
        bool isKept;

        const bool isFirst = vtkSMPTools::GetSingleThread();
        for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
        {
          if (isFirst)
          {
            this->Filter->CheckAbort();
          }
          if (this->Filter->GetAbortOutput())
          {
            break;
          }
          TableBasedPointBatch& batch = this->PointBatches[batchId];
          pointsMapValues[1] = static_cast<TInputIdType>(batch.Data.PointsOffset);
          for (pointId = batch.BeginId; pointId < batch.EndId; ++pointId)
          {
            isKept = pointsMap[pointId] > 0;
            pointsMap[pointId] = pointsMapValues[isKept];
            pointsMapValues[1] += isKept;
          }
        }
      });
  }
};

// 8 because of hexahedron.
constexpr int MAX_CELL_SIZE = 8;

//-----------------------------------------------------------------------------
// Keep track of output information within each batch of cells - this
// information is eventually rolled up into offsets into the cell
// connectivity and offsets arrays so that separate threads know where to
// write their data. We need to know how many total cells are created, the
// number of lines generated (which is equal to the number of clipped cells),
// and the connectivity size of the output cells and lines.
struct TableBasedCellBatchData
{
  // In EvaluateCells::operator() this is used as an accumulator
  // in EvaluateCells::Reduce() this is changed to an offset
  // This is done to reduce memory footprint.
  vtkIdType CellsOffset;
  vtkIdType CellsConnectivityOffset;
  vtkIdType CentroidsOffset;

  TableBasedCellBatchData()
    : CellsOffset(0)
    , CellsConnectivityOffset(0)
    , CentroidsOffset(0)
  {
  }
  ~TableBasedCellBatchData() = default;
  TableBasedCellBatchData& operator+=(const TableBasedCellBatchData& other)
  {
    this->CellsOffset += other.CellsOffset;
    this->CellsConnectivityOffset += other.CellsConnectivityOffset;
    this->CentroidsOffset += other.CentroidsOffset;
    return *this;
  }
  TableBasedCellBatchData operator+(const TableBasedCellBatchData& other) const
  {
    TableBasedCellBatchData result = *this;
    result += other;
    return result;
  }
};
using TableBasedCellBatch = vtkBatch<TableBasedCellBatchData>;
using TableBasedCellBatches = vtkBatches<TableBasedCellBatchData>;

//-----------------------------------------------------------------------------
// An Edge with its two points and a percentage value
template <typename TInputIdType>
using EdgeType = EdgeTuple<TInputIdType, double>;

//-----------------------------------------------------------------------------
// Edge Locator to store and search edges
template <typename TInputIdType>
using EdgeLocatorType = vtkStaticEdgeLocatorTemplate<TInputIdType, double>;

//-----------------------------------------------------------------------------
// Evaluate unstructured cells and calculate connectivitySize, numberOfOutputCells,
// numberOfCentroids, cellBatches, cellsCase, edges
template <typename TGrid, typename TInputIdType, bool TInsideOut>
struct EvaluateCells
{
  using TBCCases = vtkTableBasedClipCases<TInsideOut>;
  using TEdge = EdgeType<TInputIdType>;

  TGrid* Input;
  vtkDoubleArray* ClipArray;
  double IsoValue;
  vtkIdType NumberOfInputCells;
  vtkTableBasedClipDataSet* Filter;

  vtkSMPThreadLocalObject<vtkIdList> TLIdList;
  vtkSMPThreadLocal<std::vector<TEdge>> TLEdges;
  vtkSMPThreadLocal<std::unordered_set<int>> TLUnsupportedCellTypes;

  TableBasedCellBatches CellBatches;
  vtkSmartPointer<vtkUnsignedCharArray> CellsCase;
  std::vector<TEdge> Edges;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfCentroids;
  std::vector<vtkIdType> UnsupportedCells;

  EvaluateCells(TGrid* input, vtkDoubleArray* clipArray, double isoValue, unsigned int batchSize,
    vtkTableBasedClipDataSet* filter)
    : Input(input)
    , ClipArray(clipArray)
    , IsoValue(isoValue)
    , NumberOfInputCells(input->GetNumberOfCells())
    , Filter(filter)
  {
    // initialize batches
    this->CellBatches.Initialize(this->NumberOfInputCells, batchSize);
    // initialize cellsCase
    this->CellsCase = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->CellsCase->SetNumberOfValues(this->NumberOfInputCells);
    // build cells for polydata so that you can use GetCellPoints()
    if (auto inputPolyData = vtkPolyData::SafeDownCast(input))
    {
      if (inputPolyData->NeedToBuildCells())
      {
        inputPolyData->BuildCells();
      }
    }
  }

  void Initialize()
  {
    // initialize list size
    this->TLIdList.Local()->Allocate(MAX_CELL_SIZE);
    // initialize edges
    this->TLEdges.Local().reserve(static_cast<size_t>(this->Input->GetNumberOfPoints() * 0.001));
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    vtkIdList* idList = this->TLIdList.Local();
    auto& edges = this->TLEdges.Local();
    auto& unsupportedCellTypes = this->TLUnsupportedCellTypes.Local();
    const auto& clipArray = vtk::DataArrayValueRange<1>(this->ClipArray);
    auto cellsCase = vtk::DataArrayValueRange<1>(this->CellsCase);
    const vtkIdType* pointIndices;
    vtkIdType numberOfPoints, cellId, pointId;
    TInputIdType pointIndex1, pointIndex2;
    int cellType;
    double grdDiffs[8], point1ToPoint2, point1ToIso, point1Weight;
    uint8_t caseIndex, *thisCase, numberOfOutputCells, outputCellId, shape, numberOfCellPoints, p;
    uint8_t pointIndex, point1Index, point2Index;
    const typename TBCCases::EDGEIDXS* edgeVertices = nullptr;

    const bool isFirst = vtkSMPTools::GetSingleThread();
    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      TableBasedCellBatch& batch = this->CellBatches[batchId];
      auto& batchNumberOfCells = batch.Data.CellsOffset;
      auto& batchCellsConnectivity = batch.Data.CellsConnectivityOffset;
      auto& batchNumberOfCentroids = batch.Data.CentroidsOffset;

      for (cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        cellType = this->Input->GetCellType(cellId);
        // check if the cell type is supported
        if (!TBCCases::IsCellTypeSupported(cellType))
        {
          if (cellType != VTK_EMPTY_CELL)
          {
            unsupportedCellTypes.insert(cellType);
          }
          // here we set that this cell is discarded
          cellsCase[cellId] = TBCCases::DISCARDED_CELL_CASE;
          continue;
        }
        this->Input->GetCellPoints(cellId, numberOfPoints, pointIndices, idList);

        // compute case index
        caseIndex = 0;
        for (pointId = numberOfPoints - 1; pointId >= 0; --pointId)
        {
          grdDiffs[pointId] = clipArray[pointIndices[pointId]] - this->IsoValue;
          caseIndex |= (grdDiffs[pointId] >= 0.0) << pointId;
        }

        // Keep the cell (Fast path)
        if (TBCCases::IsCellKept(numberOfPoints, caseIndex))
        {
          cellsCase[cellId] = caseIndex;
          batchNumberOfCells++;
          batchCellsConnectivity += numberOfPoints;
          continue;
        }
        // Discard the cell (Fast path)
        else if (TBCCases::IsCellDiscarded(numberOfPoints, caseIndex))
        {
          cellsCase[cellId] = TBCCases::DISCARDED_CELL_CASE;
          continue;
        }
        // Clip the cell
        cellsCase[cellId] = caseIndex;

        // shape case, number of outputs, and vertices from edges
        thisCase = TBCCases::GetCellCase(cellType, caseIndex);
        numberOfOutputCells = *thisCase++;
        edgeVertices = TBCCases::GetCellEdges(cellType);

        for (outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
        {
          shape = *thisCase++;
          numberOfCellPoints = *thisCase++;

          for (p = 0; p < numberOfCellPoints; ++p)
          {
            pointIndex = *thisCase++;

            if (pointIndex >= TBCCases::EA && pointIndex <= TBCCases::EL) // Mid-Edge Point
            {
              const auto& edgePoints = edgeVertices[pointIndex - TBCCases::EA];
              point1Index = edgePoints[0];
              point2Index = edgePoints[1];
              if (point1Index > point2Index)
              {
                std::swap(point1Index, point2Index);
              }

              point1ToPoint2 = grdDiffs[point2Index] - grdDiffs[point1Index];
              point1ToIso = 0.0 - grdDiffs[point1Index];
              point1Weight = 1.0 - point1ToIso / point1ToPoint2;

              pointIndex1 = static_cast<TInputIdType>(pointIndices[point1Index]);
              pointIndex2 = static_cast<TInputIdType>(pointIndices[point2Index]);
              if (pointIndex1 > pointIndex2)
              {
                std::swap(pointIndex1, pointIndex2);
                point1Weight = 1.0 - point1Weight;
              }

              edges.emplace_back(pointIndex1, pointIndex2, point1Weight);
            }
          }
          if (shape != TBCCases::ST_PNT) // normal cell
          {
            batchNumberOfCells++;
            batchCellsConnectivity += numberOfCellPoints;
          }
          else // shape == ST_PNT
          {
            batchNumberOfCentroids++;
          }
        }
      }
    }
  }

  void Reduce()
  {
    // trim batches with 0 cells in-place
    this->CellBatches.TrimBatches(
      [](const TableBasedCellBatch& batch) { return batch.Data.CellsOffset == 0; });

    // assign beginCellsOffset/BeginCellsConnectivity/BeginCentroid for each batch
    const auto globalSum = this->CellBatches.BuildOffsetsAndGetGlobalSum();
    this->NumberOfOutputCells = globalSum.CellsOffset;
    this->ConnectivitySize = globalSum.CellsConnectivityOffset;
    this->NumberOfCentroids = globalSum.CentroidsOffset;

    // store TLEdges in vector
    using TLEdgesIterator = decltype(this->TLEdges.begin());
    std::vector<TLEdgesIterator> tlEdgesVector;
    for (auto iter = this->TLEdges.begin(); iter != this->TLEdges.end(); ++iter)
    {
      tlEdgesVector.push_back(iter);
    }
    // compute total size of edges
    size_t totalSizeOfEdges = 0;
    for (auto& tlEdges : tlEdgesVector)
    {
      totalSizeOfEdges += tlEdges->size();
    }

    // compute begin indices
    std::vector<size_t> beginIndices(this->TLEdges.size(), 0);
    for (size_t i = 1; i < tlEdgesVector.size(); ++i)
    {
      beginIndices[i] = beginIndices[i - 1] + tlEdgesVector[i - 1]->size();
    }

    // merge thread local edges
    this->Edges.resize(totalSizeOfEdges);
    vtkSMPTools::For(0, static_cast<vtkIdType>(tlEdgesVector.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType threadId = begin; threadId < end; ++threadId)
        {
          auto& edges = *tlEdgesVector[threadId];
          std::copy(edges.begin(), edges.end(), this->Edges.begin() + beginIndices[threadId]);
        }
      });

    // merge not supported cell types
    std::unordered_set<int> unsupportedCellTypes;
    for (auto& tlUnsupportedCellTypes : this->TLUnsupportedCellTypes)
    {
      unsupportedCellTypes.insert(tlUnsupportedCellTypes.begin(), tlUnsupportedCellTypes.end());
    }
    this->UnsupportedCells.clear();
    // if there are any not supported cell types, find the ids of the cells
    if (!unsupportedCellTypes.empty())
    {
      // this loop is done sequentially to avoid sorting the ids later on
      // when passed to vtkExtractCells
      int cellType;
      for (vtkIdType cellId = 0; cellId < this->NumberOfInputCells; ++cellId)
      {
        cellType = this->Input->GetCellType(cellId);
        if (unsupportedCellTypes.find(cellType) != unsupportedCellTypes.end())
        {
          this->UnsupportedCells.push_back(cellId);
        }
      }
    }
  }
};

//-----------------------------------------------------------------------------
// Centroid, which saves the number of points and their point ids
// This structure could be templated to save space, but it's not because
// it's beneficial to avoid std::transform, and Interpolate later (which required vtkIdTypes)
struct Centroid
{
  vtkIdType PointIds[MAX_CELL_SIZE];
  uint8_t NumberOfPoints;

  Centroid() = default;

  template <typename TOutputIdType>
  Centroid(const TOutputIdType* pointIds, uint8_t numberOfPoints)
    : NumberOfPoints(numberOfPoints)
  {
    std::copy(pointIds, pointIds + numberOfPoints, this->PointIds);
  }
};

//-----------------------------------------------------------------------------
// Extract cells unstructured
template <typename TGrid, typename TInputIdType, typename TOutputIdType, bool TInsideOut>
struct ExtractCells
{
  using TBCCases = vtkTableBasedClipCases<TInsideOut>;
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  using TOutputIdTypeArray = vtkAOSDataArrayTemplate<TOutputIdType>;

  TGrid* Input;
  vtkAOSDataArrayTemplate<TInputIdType>* PointsMap;
  vtkUnsignedCharArray* CellsCase;
  const TableBasedCellBatches& CellBatches;
  ArrayList& CellDataArrays;
  const TEdgeLocator& EdgeLocator;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfKeptPoints;
  vtkIdType NumberOfEdges;
  vtkIdType NumberOfCentroids;
  vtkIdType NumberOfKeptPointsAndEdges;
  vtkTableBasedClipDataSet* Filter;

  vtkSMPThreadLocalObject<vtkIdList> TLIdList;

  vtkSmartPointer<TOutputIdTypeArray> Connectivity;
  vtkSmartPointer<TOutputIdTypeArray> Offsets;

  std::vector<Centroid> Centroids;
  vtkSmartPointer<vtkUnsignedCharArray> OutputCellTypes;
  vtkSmartPointer<vtkCellArray> OutputCellArray;

  ExtractCells(TGrid* input, vtkAOSDataArrayTemplate<TInputIdType>* pointsMap,
    vtkUnsignedCharArray* cellsCase, const TableBasedCellBatches& cellBatches,
    ArrayList& cellDataArrays, const TEdgeLocator& edgeLocator, vtkIdType connectivitySize,
    vtkIdType numberOfOutputCells, vtkIdType numberOfKeptPoints, vtkIdType numberOfEdges,
    vtkIdType numberOfCentroids, vtkTableBasedClipDataSet* filter)
    : Input(input)
    , PointsMap(pointsMap)
    , CellsCase(cellsCase)
    , CellBatches(cellBatches)
    , CellDataArrays(cellDataArrays)
    , EdgeLocator(edgeLocator)
    , ConnectivitySize(connectivitySize)
    , NumberOfOutputCells(numberOfOutputCells)
    , NumberOfKeptPoints(numberOfKeptPoints)
    , NumberOfEdges(numberOfEdges)
    , NumberOfCentroids(numberOfCentroids)
    , NumberOfKeptPointsAndEdges(numberOfKeptPoints + numberOfEdges)
    , Filter(filter)
  {
    // create connectivity array, offsets array, and types array
    this->Connectivity = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Connectivity->SetNumberOfValues(this->ConnectivitySize);
    this->Offsets = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Offsets->SetNumberOfValues(this->NumberOfOutputCells + 1);
    this->OutputCellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->OutputCellTypes->SetNumberOfValues(this->NumberOfOutputCells);
    // initialize centroids
    this->Centroids.resize(this->NumberOfCentroids);
  }

  void Initialize()
  {
    // initialize list size
    this->TLIdList.Local()->Allocate(MAX_CELL_SIZE);
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    vtkIdList*& idList = this->TLIdList.Local();
    const auto& pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap);
    const auto& cellsCase = vtk::DataArrayValueRange<1>(this->CellsCase);
    auto connectivity = this->Connectivity->GetPointer(0);
    auto offsets = vtk::DataArrayValueRange<1>(this->Offsets);
    auto types = vtk::DataArrayValueRange<1>(this->OutputCellTypes);
    const vtkIdType* pointIndices;
    vtkIdType numberOfPoints, cellId, pointId, centroidIndex = -1;
    TInputIdType pointIndex1, pointIndex2;
    TOutputIdType shapeIds[MAX_CELL_SIZE];
    int cellType;
    uint8_t *thisCase, numberOfOutputCells, shape, outputCellId, numberOfCellPoints, p;
    uint8_t pointIndex;
    const typename TBCCases::EDGEIDXS* edgeVertices = nullptr;
    // Used to map the voxel/pixel indices to the hexahedron/quad indices
    static constexpr uint8_t voxelMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };

    const bool isFirst = vtkSMPTools::GetSingleThread();
    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      const TableBasedCellBatch& batch = this->CellBatches[batchId];
      auto cellsOffset = batch.Data.CellsOffset;
      auto cellsConnectivityOffset = static_cast<TOutputIdType>(batch.Data.CellsConnectivityOffset);
      auto centroidsOffset = batch.Data.CentroidsOffset;

      for (cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        // process cells that has output cells (either itself or at least because it's clipped)
        const auto& caseIndex = cellsCase[cellId];
        // Discard the cell (Fast path without using numberOfPoints)
        if (caseIndex == TBCCases::DISCARDED_CELL_CASE)
        {
          continue;
        }

        cellType = this->Input->GetCellType(cellId);
        this->Input->GetCellPoints(cellId, numberOfPoints, pointIndices, idList);

        // Keep the cell (Fast path)
        if (TBCCases::IsCellKept(numberOfPoints, caseIndex))
        {
          offsets[cellsOffset] = cellsConnectivityOffset;
          switch (cellType)
          {
            case VTK_VOXEL:
            case VTK_PIXEL:
              for (pointId = 0; pointId < numberOfPoints; ++pointId)
              {
                connectivity[cellsConnectivityOffset++] =
                  static_cast<TOutputIdType>(pointsMap[pointIndices[voxelMap[pointId]]]);
              }
              // change cell type to VTK_HEXAHEDRON/VTK_QUAD
              types[cellsOffset] = cellType + 1;
              break;
            default:
              for (pointId = 0; pointId < numberOfPoints; ++pointId)
              {
                connectivity[cellsConnectivityOffset++] =
                  static_cast<TOutputIdType>(pointsMap[pointIndices[pointId]]);
              }
              types[cellsOffset] = cellType;
              break;
          }
          this->CellDataArrays.Copy(cellId, cellsOffset++);
          continue;
        }
        // Clip the cell

        // shape case, number of outputs, and vertices from edges
        thisCase = TBCCases::GetCellCase(cellType, caseIndex);
        numberOfOutputCells = *thisCase++;
        edgeVertices = TBCCases::GetCellEdges(cellType);

        for (outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
        {
          shape = *thisCase++;
          numberOfCellPoints = *thisCase++;

          for (p = 0; p < numberOfCellPoints; ++p)
          {
            pointIndex = *thisCase++;

            if (pointIndex <= TBCCases::P7) // Input Point
            {
              // We know pt P0 must be > P0 since we already
              // assume P0 == 0.  This is why we do not
              // bother subtracting P0 from pt here.
              shapeIds[p] = static_cast<TOutputIdType>(pointsMap[pointIndices[pointIndex]]);
            }
            else if (/*pointIndex >= TBCCases::EA &&*/ pointIndex <= TBCCases::EL) // Mid-Edge Point
            {
              const auto& edgePoints = edgeVertices[pointIndex - TBCCases::EA];
              pointIndex1 = static_cast<TInputIdType>(pointIndices[edgePoints[0]]);
              pointIndex2 = static_cast<TInputIdType>(pointIndices[edgePoints[1]]);

              shapeIds[p] = static_cast<TOutputIdType>(this->NumberOfKeptPoints +
                this->EdgeLocator.IsInsertedEdge(pointIndex1, pointIndex2));
            }
            else // pointIndex == TBCCases::N0 // Centroid Point
            {
              shapeIds[p] = static_cast<TOutputIdType>(centroidIndex);
            }
          }

          switch (shape)
          {
            case TBCCases::ST_HEX:
              types[cellsOffset] = VTK_HEXAHEDRON;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 8 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 8;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_WDG:
              types[cellsOffset] = VTK_WEDGE;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 6 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 6;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_PYR:
              types[cellsOffset] = VTK_PYRAMID;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 5 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 5;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_TET:
              types[cellsOffset] = VTK_TETRA;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 4 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 4;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_QUA:
              types[cellsOffset] = VTK_QUAD;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 4 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 4;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_TRI:
              types[cellsOffset] = VTK_TRIANGLE;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 3 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 3;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_LIN:
              types[cellsOffset] = VTK_LINE;
              offsets[cellsOffset] = cellsConnectivityOffset;
              std::memcpy(connectivity + offsets[cellsOffset], shapeIds, 2 * sizeof(TOutputIdType));
              cellsConnectivityOffset += 2;
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_VTX:
              types[cellsOffset] = VTK_VERTEX;
              offsets[cellsOffset] = cellsConnectivityOffset;
              connectivity[cellsConnectivityOffset++] = shapeIds[0];
              this->CellDataArrays.Copy(cellId, cellsOffset++);
              break;

            case TBCCases::ST_PNT:
              this->Centroids[centroidsOffset] = Centroid(shapeIds, numberOfCellPoints);
              centroidIndex = this->NumberOfKeptPointsAndEdges + centroidsOffset++;
          }
        }
      }
    }
  }

  void Reduce()
  {
    // assign last offset
    this->Offsets->SetValue(this->NumberOfOutputCells, this->ConnectivitySize);
    // create cell array
    this->OutputCellArray = vtkSmartPointer<vtkCellArray>::New();
    this->OutputCellArray->SetData(this->Offsets, this->Connectivity);
  }
};

//-----------------------------------------------------------------------------
// Extract points
template <typename TInputIdType>
struct ExtractPointsWorker
{
  using TEdge = EdgeType<TInputIdType>;

  template <typename TInputPoints, typename TOutputPoints>
  void operator()(TInputPoints* inputPoints, TOutputPoints* outputPoints,
    const TableBasedPointBatches& pointBatches, vtkAOSDataArrayTemplate<TInputIdType>* pointsMap,
    ArrayList& pointDataArrays, const std::vector<TEdge>& edges,
    const std::vector<Centroid>& centroids, vtkIdType numberOfKeptPoints, vtkIdType numberOfEdges,
    vtkIdType numberOfCentroids, vtkTableBasedClipDataSet* filter)
  {
    const auto inPts = vtk::DataArrayTupleRange<3>(inputPoints);
    auto outPts = vtk::DataArrayTupleRange<3>(outputPoints);
    const auto ptsMap = vtk::DataArrayValueRange<1>(pointsMap);

    // copy kept input points
    auto extractKeptPoints = [&](vtkIdType beginBatchId, vtkIdType endBatchId)
    {
      vtkIdType pointId;
      double inputPoint[3];

      const bool isFirst = vtkSMPTools::GetSingleThread();
      for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
      {
        if (isFirst)
        {
          filter->CheckAbort();
        }
        if (filter->GetAbortOutput())
        {
          break;
        }
        const TableBasedPointBatch& batch = pointBatches[batchId];
        for (pointId = batch.BeginId; pointId < batch.EndId; ++pointId)
        {
          if (ptsMap[pointId] >= 0)
          {
            const auto& keptPointId = ptsMap[pointId];
            // GetTuple creates a copy of the tuple using GetTypedTuple if it's not a vktDataArray
            // we do that since the input points can be implicit points, and GetTypedTuple is faster
            // than accessing the component of the TupleReference using GetTypedComponent
            // internally.
            inPts.GetTuple(pointId, inputPoint);
            auto outputPoint = outPts[keptPointId];
            outputPoint[0] = inputPoint[0];
            outputPoint[1] = inputPoint[1];
            outputPoint[2] = inputPoint[2];
            pointDataArrays.Copy(pointId, keptPointId);
          }
        }
      }
    };
    vtkSMPTools::For(0, pointBatches.GetNumberOfBatches(), extractKeptPoints);

    // create edge points
    auto extractEdgePoints = [&](vtkIdType beginEdgeId, vtkIdType endEdgeId)
    {
      vtkIdType outputEdgePointId;
      double edgePoint1[3], edgePoint2[3];

      const bool isFirst = vtkSMPTools::GetSingleThread();
      const auto checkAbortInterval = std::min((endEdgeId - beginEdgeId) / 10 + 1, (vtkIdType)1000);
      for (vtkIdType edgeId = beginEdgeId; edgeId < endEdgeId; ++edgeId)
      {
        if (edgeId % checkAbortInterval == 0)
        {
          if (isFirst)
          {
            filter->CheckAbort();
          }
          if (filter->GetAbortOutput())
          {
            break;
          }
        }
        const TEdge& edge = edges[edgeId];
        // GetTuple creates a copy of the tuple using GetTypedTuple if it's not a vktDataArray
        // we do that since the input points can be implicit points, and GetTypedTuple is faster
        // than accessing the component of the TupleReference using GetTypedComponent internally.
        inPts.GetTuple(edge.V0, edgePoint1);
        inPts.GetTuple(edge.V1, edgePoint2);
        outputEdgePointId = numberOfKeptPoints + edgeId;
        auto outputPoint = outPts[outputEdgePointId];

        const double& percentage = edge.Data;
        const double bPercentage = 1.0 - percentage;
        outputPoint[0] = edgePoint1[0] * percentage + edgePoint2[0] * bPercentage;
        outputPoint[1] = edgePoint1[1] * percentage + edgePoint2[1] * bPercentage;
        outputPoint[2] = edgePoint1[2] * percentage + edgePoint2[2] * bPercentage;
        pointDataArrays.InterpolateEdge(edge.V0, edge.V1, bPercentage, outputEdgePointId);
      }
    };
    vtkSMPTools::For(0, numberOfEdges, extractEdgePoints);

    // create centroid points
    auto extractCentroids = [&](vtkIdType beginCentroid, vtkIdType endCentroid)
    {
      vtkIdType outputCentroidPointId;
      double weights[MAX_CELL_SIZE];
      double weightFactor;

      const bool isFirst = vtkSMPTools::GetSingleThread();
      const auto checkAbortInterval =
        std::min((endCentroid - beginCentroid) / 10 + 1, (vtkIdType)1000);
      for (vtkIdType centroidId = beginCentroid; centroidId < endCentroid; ++centroidId)
      {
        if (centroidId % checkAbortInterval == 0)
        {
          if (isFirst)
          {
            filter->CheckAbort();
          }
          if (filter->GetAbortOutput())
          {
            break;
          }
        }
        const Centroid& centroid = centroids[centroidId];
        outputCentroidPointId = numberOfKeptPoints + numberOfEdges + centroidId;
        auto outputPoint = outPts[outputCentroidPointId];

        outputPoint[0] = 0;
        outputPoint[1] = 0;
        outputPoint[2] = 0;
        weightFactor = 1.0 / centroid.NumberOfPoints;
        for (uint8_t i = 0; i < centroid.NumberOfPoints; ++i)
        {
          const auto& iOutputPoint = outPts[centroid.PointIds[i]];
          outputPoint[0] += iOutputPoint[0];
          outputPoint[1] += iOutputPoint[1];
          outputPoint[2] += iOutputPoint[2];
          weights[i] = 1.0 * weightFactor;
        }
        outputPoint[0] *= weightFactor;
        outputPoint[1] *= weightFactor;
        outputPoint[2] *= weightFactor;
        pointDataArrays.InterpolateOutput(static_cast<int>(centroid.NumberOfPoints),
          centroid.PointIds, weights, outputCentroidPointId);
      }
    };
    vtkSMPTools::For(0, numberOfCentroids, extractCentroids);
  }
};
} // end anonymous namespace

//-----------------------------------------------------------------------------
template <typename TGrid, typename TInputIdType, bool TInsideOut>
vtkSmartPointer<vtkUnstructuredGrid> vtkTableBasedClipDataSet::ClipTDataSet(
  TGrid* input, vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue)
{
  const auto inputPoints = input->GetPoints();
  vtkSmartPointer<vtkDoubleArray> clipArray;
  if (implicitFunction)
  {
    clipArray = vtkSmartPointer<vtkDoubleArray>::New();
    clipArray->SetName("ClipDataSetScalars");
    clipArray->SetNumberOfComponents(1);
    clipArray->SetNumberOfTuples(inputPoints->GetNumberOfPoints());
    implicitFunction->FunctionValue(inputPoints->GetData(), clipArray);
  }
  else
  {
    clipArray = scalars;
  }
  // Evaluate points and calculate pointBatches, numberOfKeptPoints, pointsMap using clipArray
  EvaluatePoints<TInputIdType, TInsideOut> evaluatePoints(
    clipArray, isoValue, this->BatchSize, this);
  vtkSMPTools::For(0, evaluatePoints.PointBatches.GetNumberOfBatches(), evaluatePoints);
  const TInputIdType numberOfKeptPoints = evaluatePoints.NumberOfKeptPoints;
  const TableBasedPointBatches& pointBatches = evaluatePoints.PointBatches;
  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> pointsMap = evaluatePoints.PointsMap;
  if (implicitFunction && this->GenerateClipScalars)
  {
    input->GetPointData()->AddArray(clipArray);
    input->GetPointData()->SetActiveScalars(clipArray->GetName());
  }
  // check if there are no kept points
  if (numberOfKeptPoints == 0)
  {
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  // Evaluate cells and calculate connectivitySize, numberOfOutputCells, numberOfCentroids,
  // cellBatches, cellsCase, edges
  using TEdge = EdgeType<TInputIdType>;
  EvaluateCells<TGrid, TInputIdType, TInsideOut> evaluateCells(
    input, clipArray.Get(), isoValue, this->BatchSize, this);
  vtkSMPTools::For(0, evaluateCells.CellBatches.GetNumberOfBatches(), evaluateCells);
  const vtkIdType connectivitySize = evaluateCells.ConnectivitySize;
  const vtkIdType numberOfOutputCells = evaluateCells.NumberOfOutputCells;
  const vtkIdType numberOfCentroids = evaluateCells.NumberOfCentroids;
  const TableBasedCellBatches& cellBatches = evaluateCells.CellBatches;
  vtkSmartPointer<vtkUnsignedCharArray> cellsCase = evaluateCells.CellsCase;
  std::vector<TEdge> edges = std::move(evaluateCells.Edges);
  std::vector<vtkIdType> unsupportedCells = std::move(evaluateCells.UnsupportedCells);

  // Create Edge locator which will be used to define the connectivity of cells
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  TEdgeLocator edgeLocator;
  if (!edges.empty())
  {
    edgeLocator.BuildLocator(static_cast<vtkIdType>(edges.size()), edges.data());
  }
  const TInputIdType numberOfEdges = edgeLocator.GetNumberOfEdges();

  // Calculate total number of output points
  const vtkIdType numberOfOutputPoints = numberOfKeptPoints + numberOfEdges + numberOfCentroids;

  // Initialize outputPoints
  auto outputPoints = vtkSmartPointer<vtkPoints>::New();
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outputPoints->SetDataType(inputPoints->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outputPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outputPoints->SetDataType(VTK_DOUBLE);
  }
  outputPoints->SetNumberOfPoints(numberOfOutputPoints);
  // initialize outputPointData
  auto outputPointData = vtkSmartPointer<vtkPointData>::New();
  ArrayList pointDataArrays;
  outputPointData->InterpolateAllocate(input->GetPointData(), numberOfOutputPoints);
  pointDataArrays.AddArrays(numberOfOutputPoints, input->GetPointData(), outputPointData,
    /*nullValue*/ 0.0, /*promote*/ false);
  // define outputCellTypes, outputCellArray
  vtkSmartPointer<vtkUnsignedCharArray> outputCellTypes;
  vtkSmartPointer<vtkCellArray> outputCellArray;
  // initialize outputCellData
  auto outputCellData = vtkSmartPointer<vtkCellData>::New();
  ArrayList cellDataArrays;
  outputCellData->CopyAllocate(input->GetCellData(), numberOfOutputCells);
  cellDataArrays.AddArrays(numberOfOutputCells, input->GetCellData(), outputCellData,
    /*nullValue*/ 0.0, /*promote*/ false);

  // identify the required output id type
  std::vector<Centroid> centroids;
#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds =
    (connectivitySize > VTK_TYPE_INT32_MAX || numberOfOutputPoints > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    // Extract cells and calculate centroids, types, cell array, cell data
    ExtractCells<TGrid, TInputIdType, TOutputIdType, TInsideOut> extractCells(input,
      pointsMap.Get(), cellsCase.Get(), cellBatches, cellDataArrays, edgeLocator, connectivitySize,
      numberOfOutputCells, numberOfKeptPoints, numberOfEdges, numberOfCentroids, this);
    vtkSMPTools::For(0, extractCells.CellBatches.GetNumberOfBatches(), extractCells);
    centroids = std::move(extractCells.Centroids);
    outputCellTypes = extractCells.OutputCellTypes;
    outputCellArray = extractCells.OutputCellArray;
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    // Extract cells and calculate centroids, types, cell array, cell data
    ExtractCells<TGrid, TInputIdType, TOutputIdType, TInsideOut> extractCells(input,
      pointsMap.Get(), cellsCase.Get(), cellBatches, cellDataArrays, edgeLocator, connectivitySize,
      numberOfOutputCells, numberOfKeptPoints, numberOfEdges, numberOfCentroids, this);
    vtkSMPTools::For(0, extractCells.CellBatches.GetNumberOfBatches(), extractCells);
    centroids = std::move(extractCells.Centroids);
    outputCellTypes = extractCells.OutputCellTypes;
    outputCellArray = extractCells.OutputCellArray;
  }
  // Extract points and calculate outputPoints and outputPointData.
  ExtractPointsWorker<TInputIdType> extractPointsWorker;
  using ExtractPointsDispatcher =
    vtkArrayDispatch::Dispatch2ByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
      vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!ExtractPointsDispatcher::Execute(inputPoints->GetData(), outputPoints->GetData(),
        extractPointsWorker, pointBatches, pointsMap.Get(), pointDataArrays, edges, centroids,
        numberOfKeptPoints, numberOfEdges, numberOfCentroids, this))
  {
    extractPointsWorker(inputPoints->GetData(), outputPoints->GetData(), pointBatches,
      pointsMap.Get(), pointDataArrays, edges, centroids, numberOfKeptPoints, numberOfEdges,
      numberOfCentroids, this);
  }

  // create outputClippedCells
  auto outputClippedCells = vtkSmartPointer<vtkUnstructuredGrid>::New();
  outputClippedCells->SetPoints(outputPoints);
  outputClippedCells->GetPointData()->ShallowCopy(outputPointData);
  outputClippedCells->SetPolyhedralCells(outputCellTypes, outputCellArray, nullptr, nullptr);
  outputClippedCells->GetCellData()->ShallowCopy(outputCellData);

  // check if there are unsupported cell types
  if (!unsupportedCells.empty())
  {
    // extract unsupported cells
    vtkNew<vtkExtractCells> extractUnsupportedCells;
    extractUnsupportedCells->SetInputData(input);
    extractUnsupportedCells->AssumeSortedAndUniqueIdsOn();
    extractUnsupportedCells->SetCellIds(
      unsupportedCells.data(), static_cast<vtkIdType>(unsupportedCells.size()));
    extractUnsupportedCells->Update();
    auto inputUnsupportedCells = extractUnsupportedCells->GetOutput();
    // clip unsupported cells
    vtkNew<vtkUnstructuredGrid> outputClippedUnsupportedCells;
    this->ClipDataSet(inputUnsupportedCells, outputClippedUnsupportedCells);
    // append outputClippedUnsupportedCells and outputClippedCells
    vtkNew<vtkAppendFilter> appender;
    appender->AddInputData(outputClippedCells);
    appender->AddInputData(outputClippedUnsupportedCells);
    appender->Update();
    auto outputUG = vtkSmartPointer<vtkUnstructuredGrid>::New();
    outputUG->ShallowCopy(appender->GetOutput());

    return outputUG;
  }
  else
  {
    return outputClippedCells;
  }
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipPolyData(vtkPolyData* inputGrid,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue,
  vtkUnstructuredGrid* outputUG)
{
  // check if it's easily convertible to vtkUnstructuredGrid
  auto polyData = vtkPolyData::SafeDownCast(inputGrid);
  if (vtkPolyDataToUnstructuredGrid::CanBeProcessedFast(polyData))
  {
    // convert to vtkUnstructuredGrid
    //
    // It's beneficial to convert a polydata to unstructured grid for clipping because the
    // GetCellType and GetCellPoints are the most expensive functions used (excluding point/cell
    // data related functions). The vtkPolyData ones are more expensive than the vtkUnstructuredGrid
    // ones because they perform a bit operation to get the cell type and then based on that, get
    // the correct cell array and extract the cell points. This overhead turns out to increase the
    // execution time by 10%-20%.
    vtkNew<vtkPolyDataToUnstructuredGrid> converter;
    converter->SetInputData(polyData);
    converter->SetContainerAlgorithm(this);
    converter->Update();
    auto uGrid = converter->GetOutput();
    this->ClipTDataSet(uGrid, implicitFunction, scalars, isoValue, outputUG);
  }
  else
  {
    this->ClipTDataSet(inputGrid, implicitFunction, scalars, isoValue, outputUG);
  }
}

//------------------------------------------------------------------------------
template <class TGrid>
void vtkTableBasedClipDataSet::ClipTDataSet(TGrid* inputGrid, vtkImplicitFunction* implicitFunction,
  vtkDoubleArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG)
{
  vtkSmartPointer<vtkUnstructuredGrid> clippedOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
  const vtkIdType numberOfPoints = inputGrid->GetNumberOfPoints();
  if (numberOfPoints > 0)
  {
#ifdef VTK_USE_64BIT_IDS
    const bool use64BitsIds = (numberOfPoints > VTK_TYPE_INT32_MAX);
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      if (this->InsideOut)
      {
        clippedOutput = this->ClipTDataSet<TGrid, TInputIdType, true>(
          inputGrid, implicitFunction, scalars, isoValue);
      }
      else
      {
        clippedOutput = this->ClipTDataSet<TGrid, TInputIdType, false>(
          inputGrid, implicitFunction, scalars, isoValue);
      }
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      if (this->InsideOut)
      {
        clippedOutput = this->ClipTDataSet<TGrid, TInputIdType, true>(
          inputGrid, implicitFunction, scalars, isoValue);
      }
      else
      {
        clippedOutput = this->ClipTDataSet<TGrid, TInputIdType, false>(
          inputGrid, implicitFunction, scalars, isoValue);
      }
    }
  }
  outputUG->ShallowCopy(clippedOutput);
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if (this->ClipFunction)
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");
  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");
  os << indent << "UseValueAsOffset: " << (this->UseValueAsOffset ? "On\n" : "Off\n");
  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
  os << indent << "Batch size: " << this->BatchSize << "\n";
}
VTK_ABI_NAMESPACE_END
