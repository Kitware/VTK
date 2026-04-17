// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTableBasedClipDataSet.h"
#include "vtkMarchingCellsClipCases.h"

#include "vtkAppendFilter.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkArrayListTemplate.h"
#include "vtkBatch.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkClipDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkExtractCells.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataToUnstructuredGrid.h"
#include "vtkPolyhedronContour.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStructuredGrid.h"
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
  this->GenerateClipPointTypes = false;
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
  vtkSmartPointer<vtkDataArray> scalars;
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
    scalars = inputArray;
  }
  else
  {
    scalars = vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetName("ClipDataSetScalars");
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(inputCopy->GetNumberOfPoints());
    this->ClipFunction->FunctionValue(inputCopy->GetPoints()->GetData(), scalars);
    if (this->GenerateClipScalars)
    {
      inputCopy->GetPointData()->AddArray(scalars);
      inputCopy->GetPointData()->SetActiveScalars(scalars->GetName());
    }
  }

  double isoValue = (!this->ClipFunction || this->UseValueAsOffset) ? this->Value : 0.0;
  if (auto imageData = vtkImageData::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(imageData, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(imageData, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto polyData = vtkPolyData::SafeDownCast(inputCopy))
  {
    this->ClipPolyData(polyData, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipPolyData(polyData, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto recGrid = vtkRectilinearGrid::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(recGrid, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(recGrid, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto structGrid = vtkStructuredGrid::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(structGrid, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(structGrid, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto uGrid = vtkUnstructuredGrid::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(uGrid, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(uGrid, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto uGridBase = vtkUnstructuredGridBase::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(uGridBase, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(uGridBase, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (auto pointset = vtkPointSet::SafeDownCast(inputCopy))
  {
    this->ClipTDataSet(pointset, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipTDataSet(pointset, scalars, isoValue, clippedOutputUG);
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
template <typename TScalarsArray, typename TInputIdType, bool TInsideOut>
struct EvaluatePoints
{
  TScalarsArray* Scalars;
  double IsoValue;
  vtkIdType NumberOfInputPoints;
  vtkTableBasedClipDataSet* Filter;

  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> PointsMap;

  static constexpr TInputIdType IsKeptValues[2] = { -1, 1 };

  TableBasedPointBatches PointBatches;
  TInputIdType NumberOfKeptPoints;

  EvaluatePoints(TScalarsArray* scalars, double isoValue, unsigned int batchSize,
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
      for (vtkIdType pointId = batch.BeginId; pointId < batch.EndId; ++pointId)
      {
        // Outside points are marked with -1, others with 1
        const auto grdDiff = scalars[pointId] - this->IsoValue;
        const bool isKept = TInsideOut ? grdDiff < 0 : grdDiff >= 0;
        pointsMap[pointId] = IsKeptValues[isKept];
        batchNumberOfPoints += pointsMap[pointId] > 0;
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
        TInputIdType pointsMapValues[2] = { -1 /*always the same*/, 0 /*offset*/ };

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
          for (vtkIdType pointId = batch.BeginId; pointId < batch.EndId; ++pointId)
          {
            const bool isKept = pointsMap[pointId] > 0;
            pointsMap[pointId] = pointsMapValues[isKept];
            pointsMapValues[1] += isKept;
          }
        }
      });
  }
};

template <typename TInputIdType, bool TInsideOut>
struct EvaluatePointsWorker
{
  TInputIdType NumberOfKeptPoints;
  TableBasedPointBatches PointBatches;
  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> PointsMap;

  template <typename TScalarsArray>
  void operator()(TScalarsArray* scalars, double isoValue, unsigned int batchSize,
    vtkTableBasedClipDataSet* filter)
  {
    EvaluatePoints<TScalarsArray, TInputIdType, TInsideOut> evalPoints(
      scalars, isoValue, batchSize, filter);
    vtkSMPTools::For(0, evalPoints.PointBatches.GetNumberOfBatches(), evalPoints);
    this->NumberOfKeptPoints = evalPoints.NumberOfKeptPoints;
    this->PointBatches = std::move(evalPoints.PointBatches);
    this->PointsMap = std::move(evalPoints.PointsMap);
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
  vtkIdType FacesOffset;
  vtkIdType FacesConnectivityOffset;
  vtkIdType CentroidsOffset;

  TableBasedCellBatchData()
    : CellsOffset(0)
    , CellsConnectivityOffset(0)
    , FacesOffset(0)
    , FacesConnectivityOffset(0)
    , CentroidsOffset(0)
  {
  }
  ~TableBasedCellBatchData() = default;
  TableBasedCellBatchData& operator+=(const TableBasedCellBatchData& other)
  {
    this->CellsOffset += other.CellsOffset;
    this->CellsConnectivityOffset += other.CellsConnectivityOffset;
    this->FacesOffset += other.FacesOffset;
    this->FacesConnectivityOffset += other.FacesConnectivityOffset;
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
// Edge Locator to store and search edges that stores two points and a percentage value
using EdgeLocatorType = vtkStaticEdgeLocatorTemplate<vtkIdType, double>;
using EdgeType = EdgeLocatorType::EdgeTupleType;

//-----------------------------------------------------------------------------
// Evaluate unstructured cells and calculate connectivitySize, numberOfOutputCells,
// numberOfCentroids, cellBatches, cellsCase, edges
template <typename TScalarsArray, typename TGrid, typename TInputIdType, bool TInsideOut>
struct EvaluateCells
{
  using MCCases = vtkMarchingCellsClipCases<TInsideOut>;

  TScalarsArray* ClipArray;
  TGrid* Input;
  double IsoValue;
  vtkIdType NumberOfInputCells;
  vtkTableBasedClipDataSet* Filter;

  vtkSMPThreadLocalObject<vtkIdList> TLIdList;
  vtkSMPThreadLocalObject<vtkCellArray> TLPolyhedron;
  vtkSMPThreadLocal<std::vector<EdgeType>> TLEdges;
  vtkSMPThreadLocal<std::unordered_set<int>> TLUnsupportedCellTypes;

  TableBasedCellBatches CellBatches;
  vtkSmartPointer<vtkUnsignedCharArray> CellsCase;
  std::vector<EdgeType> Edges;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType FacesConnectivitySize;
  vtkIdType NumberOfOutputFaces;
  vtkIdType NumberOfCentroids;
  std::vector<vtkIdType> UnsupportedCells;

  EvaluateCells(TScalarsArray* clipArray, TGrid* input, double isoValue, unsigned int batchSize,
    vtkTableBasedClipDataSet* filter)
    : ClipArray(clipArray)
    , Input(input)
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
    this->TLIdList.Local()->Reserve(MAX_CELL_SIZE);
    // initialize edges
    this->TLEdges.Local().reserve(static_cast<size_t>(this->Input->GetNumberOfPoints() * 0.001));
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    vtkIdList* idList = this->TLIdList.Local();
    vtkCellArray* polyhedronFaces = this->TLPolyhedron.Local();
    auto& edges = this->TLEdges.Local();
    auto& unsupportedCellTypes = this->TLUnsupportedCellTypes.Local();
    const auto& clipArray = vtk::DataArrayValueRange<1>(this->ClipArray);
    auto cellsCase = vtk::DataArrayValueRange<1>(this->CellsCase);
    std::vector<EdgeType> intersectedEdges;
    const vtkIdType* pointIndices;
    vtkIdType numberOfPoints, cellId, pointId;
    TInputIdType pointIndex1, pointIndex2;
    int cellType;
    double grdDiffs[8], point1ToPoint2, point1ToIso, t;
    uint8_t caseIndex, *thisCase, numberOfOutputCells, outputCellId, shape, numberOfCellPoints, p;
    uint8_t pointIndex, point1Index, point2Index;
    const typename MCCases::EDGEIDXS* edgeVertices = nullptr;

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
      auto& batchNumberOfFaces = batch.Data.FacesOffset;
      auto& batchFacesConnectivity = batch.Data.FacesConnectivityOffset;
      auto& batchNumberOfCentroids = batch.Data.CentroidsOffset;

      for (cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        cellType = this->Input->GetCellType(cellId);
        if (cellType == VTK_POLYHEDRON)
        {
          if constexpr (std::is_same_v<TGrid, vtkUnstructuredGrid>)
          {
            this->Input->GetCellPoints(cellId, numberOfPoints, pointIndices, idList);
            // count how many points are inside
            vtkIdType insidePoints = 0;
            for (pointId = numberOfPoints - 1; pointId >= 0; --pointId)
            {
              insidePoints += (clipArray[pointIndices[pointId]] - this->IsoValue) >= 0.0;
            }
            // Keep the cell (Fast path)
            if (!TInsideOut ? insidePoints == numberOfPoints : insidePoints == 0)
            {
              cellsCase[cellId] = MCCases::KEPT_CELL_CASE;
              batchNumberOfCells++;
              batchCellsConnectivity += numberOfPoints;

              vtkIdType numberOfFaces;
              const vtkIdType* faceIndices = nullptr;
              this->Input->GetPolyhedronFaceLocations()->GetCellAtId(
                cellId, numberOfFaces, faceIndices, idList);
              batchNumberOfFaces += numberOfFaces;
              for (vtkIdType faceId = 0; faceId < numberOfFaces; ++faceId)
              {
                batchFacesConnectivity +=
                  this->Input->GetPolyhedronFaces()->GetCellSize(faceIndices[faceId]);
              }
            }
            // Discard the cell (Fast path)
            else if (!TInsideOut ? insidePoints == 0 : insidePoints == numberOfPoints)
            {
              cellsCase[cellId] = MCCases::DISCARDED_CELL_CASE;
            }
            else
            {
              this->Input->GetPolyhedronFaces(cellId, polyhedronFaces);
              vtkIdType numOutputCells, numOutputCellConnectivity, numOutputFaces,
                numOutputFacesConnectivity;
              vtkPolyhedronContour::CountClip(numberOfPoints, pointIndices, polyhedronFaces,
                this->ClipArray, this->IsoValue, TInsideOut, numOutputCells,
                numOutputCellConnectivity, numOutputFaces, numOutputFacesConnectivity,
                intersectedEdges);

              cellsCase[cellId] = 1;
              batchNumberOfCells += numOutputCells;
              batchCellsConnectivity += numOutputCellConnectivity;
              batchNumberOfFaces += numOutputFaces;
              batchFacesConnectivity += numOutputFacesConnectivity;
              edges.insert(edges.end(), intersectedEdges.begin(), intersectedEdges.end());
            }
          }
          continue;
        }
        // check if the cell type is supported
        if (!MCCases::IsCellTypeSupported(cellType))
        {
          if (cellType != VTK_EMPTY_CELL)
          {
            unsupportedCellTypes.insert(cellType);
          }
          // here we set that this cell is discarded
          cellsCase[cellId] = MCCases::DISCARDED_CELL_CASE;
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
        if (MCCases::IsCellKept(numberOfPoints, caseIndex))
        {
          cellsCase[cellId] = caseIndex;
          batchNumberOfCells++;
          batchCellsConnectivity += numberOfPoints;
          continue;
        }
        // Discard the cell (Fast path)
        else if (MCCases::IsCellDiscarded(numberOfPoints, caseIndex))
        {
          cellsCase[cellId] = MCCases::DISCARDED_CELL_CASE;
          continue;
        }
        // Clip the cell
        cellsCase[cellId] = caseIndex;

        // shape case, number of outputs, and vertices from edges
        thisCase = MCCases::GetCellCase(cellType, caseIndex);
        numberOfOutputCells = *thisCase++;
        edgeVertices = MCCases::GetCellEdges(cellType);

        for (outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
        {
          shape = *thisCase++;
          numberOfCellPoints = *thisCase++;

          for (p = 0; p < numberOfCellPoints; ++p)
          {
            pointIndex = *thisCase++;

            if (pointIndex >= MCCases::EA && pointIndex <= MCCases::EL) // Mid-Edge Point
            {
              const auto& edgePoints = edgeVertices[pointIndex - MCCases::EA];
              point1Index = edgePoints[0];
              point2Index = edgePoints[1];
              point1ToPoint2 = grdDiffs[point2Index] - grdDiffs[point1Index];
              if (point1ToPoint2 < 0)
              {
                std::swap(point1Index, point2Index);
                point1ToPoint2 = -point1ToPoint2;
              }
              point1ToIso = 0.0 - grdDiffs[point1Index];
              t = point1ToPoint2 != 0 ? point1ToIso / point1ToPoint2 : 0;

              pointIndex1 = static_cast<TInputIdType>(pointIndices[point1Index]);
              pointIndex2 = static_cast<TInputIdType>(pointIndices[point2Index]);
              // swap because edges are expected to be smallest,largest, t
              if (pointIndex1 > pointIndex2)
              {
                std::swap(pointIndex1, pointIndex2);
                t = 1.0 - t;
              }

              edges.emplace_back(pointIndex1, pointIndex2, t);
            }
          }
          if (shape != VTK_EMPTY_CELL) // normal cell
          {
            batchNumberOfCells++;
            batchCellsConnectivity += numberOfCellPoints;
          }
          else // centroid
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
    this->NumberOfOutputFaces = globalSum.FacesOffset;
    this->FacesConnectivitySize = globalSum.FacesConnectivityOffset;
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

template <typename TGrid, typename TInputIdType, bool TInsideOut>
struct EvaluateCellsWorker
{
  vtkIdType NumberOfOutputCells;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputFaces;
  vtkIdType FacesConnectivitySize;
  vtkIdType NumberOfCentroids;
  TableBasedCellBatches CellBatches;
  vtkSmartPointer<vtkUnsignedCharArray> CellsCase;
  std::vector<EdgeType> Edges;
  std::vector<vtkIdType> UnsupportedCells;

  template <typename TScalarsArray>
  void operator()(TScalarsArray* clipArray, TGrid* input, double isoValue, unsigned int batchSize,
    vtkTableBasedClipDataSet* filter)
  {
    EvaluateCells<TScalarsArray, TGrid, TInputIdType, TInsideOut> evalCells(
      clipArray, input, isoValue, batchSize, filter);
    vtkSMPTools::For(0, evalCells.CellBatches.GetNumberOfBatches(), evalCells);
    this->NumberOfOutputCells = evalCells.NumberOfOutputCells;
    this->ConnectivitySize = evalCells.ConnectivitySize;
    this->NumberOfOutputFaces = evalCells.NumberOfOutputFaces;
    this->FacesConnectivitySize = evalCells.FacesConnectivitySize;
    this->NumberOfCentroids = evalCells.NumberOfCentroids;
    this->CellBatches = std::move(evalCells.CellBatches);
    this->CellsCase = std::move(evalCells.CellsCase);
    this->Edges = std::move(evalCells.Edges);
    this->UnsupportedCells = std::move(evalCells.UnsupportedCells);
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
template <typename TGrid, typename TInputIdType, typename TOutputIdType, bool TInsideOut,
  bool CreatePolyhedrons>
struct ExtractCells
{
  using MCCases = vtkMarchingCellsClipCases<TInsideOut>;
  using TOutputIdTypeArray = vtkAOSDataArrayTemplate<TOutputIdType>;

  TGrid* Input;
  vtkAOSDataArrayTemplate<TInputIdType>* PointsMap;
  vtkDataArray* ClipArray;
  vtkUnsignedCharArray* CellsCase;
  const TableBasedCellBatches& CellBatches;
  ArrayList& CellDataArrays;
  const EdgeLocatorType& EdgeLocator;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType FacesConnectivitySize;
  vtkIdType NumberOfOutputFaces;
  vtkIdType NumberOfKeptPoints;
  vtkIdType NumberOfEdges;
  vtkIdType NumberOfCentroids;
  vtkIdType NumberOfKeptPointsAndEdges;
  vtkTableBasedClipDataSet* Filter;
  double IsoValue;

  vtkSMPThreadLocalObject<vtkCellArray> TLPolyhedron;
  vtkSMPThreadLocalObject<vtkCellArray> TLOutputCells;
  vtkSMPThreadLocalObject<vtkCellArray> TLOutputPolyhedron;
  vtkSMPThreadLocalObject<vtkIdList> TLIdList;

  vtkSmartPointer<TOutputIdTypeArray> Connectivity;
  vtkSmartPointer<TOutputIdTypeArray> Offsets;
  vtkSmartPointer<TOutputIdTypeArray> FaceLocationsConnectivity;
  vtkSmartPointer<TOutputIdTypeArray> FaceLocationsOffsets;
  vtkSmartPointer<TOutputIdTypeArray> FaceConnectivity;
  vtkSmartPointer<TOutputIdTypeArray> FaceOffsets;

  std::vector<Centroid> Centroids;
  vtkSmartPointer<vtkUnsignedCharArray> OutputCellTypes;
  vtkSmartPointer<vtkCellArray> OutputCellArray;
  vtkSmartPointer<vtkCellArray> OutputFaces;
  vtkSmartPointer<vtkCellArray> OutputFaceLocations;

  ExtractCells(TGrid* input, vtkDataArray* clipArray,
    vtkAOSDataArrayTemplate<TInputIdType>* pointsMap, vtkUnsignedCharArray* cellsCase,
    const TableBasedCellBatches& cellBatches, ArrayList& cellDataArrays,
    const EdgeLocatorType& edgeLocator, vtkIdType connectivitySize, vtkIdType numberOfOutputCells,
    vtkIdType facesConnectivitySize, vtkIdType numberOfOutputFaces, vtkIdType numberOfKeptPoints,
    vtkIdType numberOfEdges, vtkIdType numberOfCentroids, vtkTableBasedClipDataSet* filter,
    double isoValue)
    : Input(input)
    , PointsMap(pointsMap)
    , ClipArray(clipArray)
    , CellsCase(cellsCase)
    , CellBatches(cellBatches)
    , CellDataArrays(cellDataArrays)
    , EdgeLocator(edgeLocator)
    , ConnectivitySize(connectivitySize)
    , NumberOfOutputCells(numberOfOutputCells)
    , FacesConnectivitySize(facesConnectivitySize)
    , NumberOfOutputFaces(numberOfOutputFaces)
    , NumberOfKeptPoints(numberOfKeptPoints)
    , NumberOfEdges(numberOfEdges)
    , NumberOfCentroids(numberOfCentroids)
    , NumberOfKeptPointsAndEdges(numberOfKeptPoints + numberOfEdges)
    , Filter(filter)
    , IsoValue(isoValue)
  {
    // create connectivity array, offsets array, and types array
    this->Connectivity = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Connectivity->SetNumberOfValues(this->ConnectivitySize);
    this->Offsets = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Offsets->SetNumberOfValues(this->NumberOfOutputCells + 1);
    if constexpr (CreatePolyhedrons)
    {
      this->FaceLocationsConnectivity = vtkSmartPointer<TOutputIdTypeArray>::New();
      this->FaceLocationsConnectivity->SetNumberOfValues(this->NumberOfOutputFaces);
      this->FaceLocationsOffsets = vtkSmartPointer<TOutputIdTypeArray>::New();
      this->FaceLocationsOffsets->SetNumberOfValues(this->NumberOfOutputCells + 1);

      this->FaceConnectivity = vtkSmartPointer<TOutputIdTypeArray>::New();
      this->FaceConnectivity->SetNumberOfValues(this->FacesConnectivitySize);
      this->FaceOffsets = vtkSmartPointer<TOutputIdTypeArray>::New();
      this->FaceOffsets->SetNumberOfValues(this->NumberOfOutputFaces + 1);
    }
    this->OutputCellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->OutputCellTypes->SetNumberOfValues(this->NumberOfOutputCells);
    // initialize centroids
    this->Centroids.resize(this->NumberOfCentroids);
  }

  void Initialize()
  {
    // initialize list size
    this->TLIdList.Local()->Reserve(MAX_CELL_SIZE);
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    vtkIdList*& idList = this->TLIdList.Local();
    vtkCellArray*& polyhedronFaces = this->TLPolyhedron.Local();
    vtkCellArray*& outputCells = this->TLOutputCells.Local();
    vtkCellArray*& outputPolyhedronFaces = this->TLOutputPolyhedron.Local();
    const auto& pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap);
    const auto& cellsCase = vtk::DataArrayValueRange<1>(this->CellsCase);
    auto connectivity = this->Connectivity->GetPointer(0);
    auto offsets = vtk::DataArrayValueRange<1>(this->Offsets);
    TOutputIdType* faceLocationsConnectivity = nullptr;
    TOutputIdType* faceLocationsOffsets = nullptr;
    TOutputIdType* faceConnectivity = nullptr;
    TOutputIdType* faceOffsets = nullptr;
    if constexpr (CreatePolyhedrons)
    {
      faceLocationsConnectivity = this->FaceLocationsConnectivity->GetPointer(0);
      faceLocationsOffsets = this->FaceLocationsOffsets->GetPointer(0);
      faceConnectivity = this->FaceConnectivity->GetPointer(0);
      faceOffsets = this->FaceOffsets->GetPointer(0);
    }
    auto types = vtk::DataArrayValueRange<1>(this->OutputCellTypes);
    const vtkIdType* pointIndices;
    vtkIdType numberOfPoints, cellId, pointId, centroidIndex = -1;
    TInputIdType pointIndex1, pointIndex2;
    TOutputIdType shapeIds[MAX_CELL_SIZE];
    int cellType;
    uint8_t *thisCase, numberOfOutputCells, shape, outputCellId, numberOfCellPoints, p;
    uint8_t pointIndex;
    const typename MCCases::EDGEIDXS* edgeVertices = nullptr;
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
      auto facesOffset = batch.Data.FacesOffset;
      auto facesConnectivityOffset = batch.Data.FacesConnectivityOffset;
      auto centroidsOffset = batch.Data.CentroidsOffset;

      for (cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        // process cells that has output cells (either itself or at least because it's clipped)
        const auto& caseIndex = cellsCase[cellId];
        // Discard the cell (Fast path without using numberOfPoints)
        if (caseIndex == MCCases::DISCARDED_CELL_CASE)
        {
          continue;
        }

        cellType = this->Input->GetCellType(cellId);
        this->Input->GetCellPoints(cellId, numberOfPoints, pointIndices, idList);

        if (cellType == VTK_POLYHEDRON)
        {
          if constexpr (std::is_same_v<TGrid, vtkUnstructuredGrid> && CreatePolyhedrons)
          {
            // Keep the cell (Fast path)
            if (caseIndex == MCCases::KEPT_CELL_CASE)
            {
              offsets[cellsOffset] = cellsConnectivityOffset;
              faceLocationsOffsets[cellsOffset] = facesOffset;
              for (pointId = 0; pointId < numberOfPoints; ++pointId)
              {
                connectivity[cellsConnectivityOffset++] =
                  static_cast<TOutputIdType>(pointsMap[pointIndices[pointId]]);
              }
              types[cellsOffset] = cellType;
              this->Input->GetPolyhedronFaces(cellId, polyhedronFaces);
              const vtkIdType numberOfFaces = polyhedronFaces->GetNumberOfCells();
              for (vtkIdType faceId = 0; faceId < numberOfFaces; ++faceId)
              {
                const vtkIdType globalFaceId = facesOffset++;
                faceOffsets[globalFaceId] = facesConnectivityOffset;
                faceLocationsConnectivity[globalFaceId] = globalFaceId;
                polyhedronFaces->GetCellAtId(faceId, numberOfPoints, pointIndices, idList);
                for (pointId = 0; pointId < numberOfPoints; ++pointId)
                {
                  faceConnectivity[facesConnectivityOffset++] =
                    static_cast<TOutputIdType>(pointsMap[pointIndices[pointId]]);
                }
              }
              this->CellDataArrays.Copy(cellId, cellsOffset++);
            }
            // Clip the cell
            else
            {
              this->Input->GetPolyhedronFaces(cellId, polyhedronFaces);
              vtkPolyhedronContour::EmitClip(numberOfPoints, pointIndices, polyhedronFaces,
                this->ClipArray, this->IsoValue, TInsideOut, this->PointsMap,
                this->NumberOfKeptPoints, this->EdgeLocator, outputCells, outputPolyhedronFaces);

              if (outputCells->GetNumberOfCells() == 1)
              {
                offsets[cellsOffset] = cellsConnectivityOffset;
                faceLocationsOffsets[cellsOffset] = facesOffset;
                outputCells->GetCellAtId(0, numberOfPoints, pointIndices, idList);
                std::copy_n(pointIndices, numberOfPoints, connectivity + cellsConnectivityOffset);
                cellsConnectivityOffset += numberOfPoints;
                types[cellsOffset] = VTK_POLYHEDRON;

                const vtkIdType numberOfFaces = outputPolyhedronFaces->GetNumberOfCells();
                for (vtkIdType faceId = 0; faceId < numberOfFaces; ++faceId)
                {
                  const vtkIdType globalFaceId = facesOffset++;
                  faceOffsets[globalFaceId] = facesConnectivityOffset;
                  faceLocationsConnectivity[globalFaceId] = globalFaceId;
                  outputPolyhedronFaces->GetCellAtId(faceId, numberOfPoints, pointIndices, idList);
                  for (pointId = 0; pointId < numberOfPoints; ++pointId)
                  {
                    // EmitClip writes output point IDs directly into outputPolyhedronFaces
                    // (both surviving vertices via pointMap lookup and iso-vertices via
                    // numberOfKeptPoints + edgeId). Copy without an additional pointsMap
                    // application, which would be wrong for iso-vertices and redundant
                    // for surviving vertices.
                    faceConnectivity[facesConnectivityOffset++] =
                      static_cast<TOutputIdType>(pointIndices[pointId]);
                  }
                }
                this->CellDataArrays.Copy(cellId, cellsOffset++);
              }
            }
          }
          continue;
        }
        // Keep the cell (Fast path)
        if (MCCases::IsCellKept(numberOfPoints, caseIndex))
        {
          offsets[cellsOffset] = cellsConnectivityOffset;
          if constexpr (CreatePolyhedrons)
          {
            faceLocationsOffsets[cellsOffset] = facesOffset;
          }
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
        thisCase = MCCases::GetCellCase(cellType, caseIndex);
        numberOfOutputCells = *thisCase++;
        edgeVertices = MCCases::GetCellEdges(cellType);

        for (outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
        {
          shape = *thisCase++;
          numberOfCellPoints = *thisCase++;

          for (p = 0; p < numberOfCellPoints; ++p)
          {
            pointIndex = *thisCase++;

            if (pointIndex <= MCCases::P7) // Input Point
            {
              // We know pt P0 must be > P0 since we already
              // assume P0 == 0.  This is why we do not
              // bother subtracting P0 from pt here.
              shapeIds[p] = static_cast<TOutputIdType>(pointsMap[pointIndices[pointIndex]]);
            }
            else if (/*pointIndex >= MCCases::EA &&*/ pointIndex <= MCCases::EL) // Mid-Edge Point
            {
              const auto& edgePoints = edgeVertices[pointIndex - MCCases::EA];
              pointIndex1 = static_cast<TInputIdType>(pointIndices[edgePoints[0]]);
              pointIndex2 = static_cast<TInputIdType>(pointIndices[edgePoints[1]]);

              shapeIds[p] = static_cast<TOutputIdType>(this->NumberOfKeptPoints +
                this->EdgeLocator.IsInsertedEdge(pointIndex1, pointIndex2));
            }
            else // pointIndex == MCCases::N0 // Centroid Point
            {
              shapeIds[p] = static_cast<TOutputIdType>(centroidIndex);
            }
          }

          if (shape != VTK_EMPTY_CELL) // normal cell
          {
            types[cellsOffset] = shape;
            offsets[cellsOffset] = cellsConnectivityOffset;
            std::copy_n(shapeIds, numberOfCellPoints, connectivity + offsets[cellsOffset]);
            cellsConnectivityOffset += numberOfCellPoints;
            if constexpr (CreatePolyhedrons)
            {
              faceLocationsOffsets[cellsOffset] = facesOffset;
            }
            this->CellDataArrays.Copy(cellId, cellsOffset++);
          }
          else // centroid
          {
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
    if (CreatePolyhedrons)
    {
      // assign last offset
      this->FaceLocationsOffsets->SetValue(this->NumberOfOutputCells, this->NumberOfOutputFaces);
      this->FaceOffsets->SetValue(this->NumberOfOutputFaces, this->FacesConnectivitySize);
      // create cell array
      this->OutputFaceLocations = vtkSmartPointer<vtkCellArray>::New();
      this->OutputFaceLocations->SetData(
        this->FaceLocationsOffsets, this->FaceLocationsConnectivity);
      this->OutputFaces = vtkSmartPointer<vtkCellArray>::New();
      this->OutputFaces->SetData(this->FaceOffsets, this->FaceConnectivity);
    }
  }
};

//-----------------------------------------------------------------------------
// Extract points
template <typename TInputIdType>
struct ExtractPointsWorker
{
  template <typename TInputPoints, typename TOutputPoints>
  void operator()(TInputPoints* inputPoints, TOutputPoints* outputPoints,
    const TableBasedPointBatches& pointBatches, vtkAOSDataArrayTemplate<TInputIdType>* pointsMap,
    ArrayList& pointDataArrays, const std::vector<EdgeType>& edges,
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
        const EdgeType& edge = edges[edgeId];
        auto v0 = edge.V0;
        auto v1 = edge.V1;
        auto t = edge.Data;
        // edges are expected to be smallest,largest, t, and t may be swapped to satisfy that
        // therefore, swap because t is expected to be in [0,1]
        if (t < 0 || t > 1)
        {
          std::swap(v0, v1);
          t = 1.0 - t;
        }
        // GetTuple creates a copy of the tuple using GetTypedTuple if it's not a vktDataArray
        // we do that since the input points can be implicit points, and GetTypedTuple is faster
        // than accessing the component of the TupleReference using GetTypedComponent internally.
        inPts.GetTuple(v0, edgePoint1);
        inPts.GetTuple(v1, edgePoint2);
        outputEdgePointId = numberOfKeptPoints + edgeId;
        auto outputPoint = outPts[outputEdgePointId];

        outputPoint[0] = edgePoint1[0] + t * (edgePoint2[0] - edgePoint1[0]);
        outputPoint[1] = edgePoint1[1] + t * (edgePoint2[1] - edgePoint1[1]);
        outputPoint[2] = edgePoint1[2] + t * (edgePoint2[2] - edgePoint1[2]);
        pointDataArrays.InterpolateEdge(v0, v1, t, outputEdgePointId);
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
  TGrid* input, vtkDataArray* clipArray, double isoValue)
{
  const auto inputPoints = input->GetPoints();
  // Evaluate points and calculate pointBatches, numberOfKeptPoints, pointsMap using clipArray
  EvaluatePointsWorker<TInputIdType, TInsideOut> evaluatePoints;
  using ScalarsDispatcher =
    vtkArrayDispatch::DispatchByArrayAndValueType<vtkArrayDispatch::AOSArrays,
      vtkArrayDispatch::Reals>;
  if (!ScalarsDispatcher::Execute(clipArray, evaluatePoints, isoValue, this->BatchSize, this))
  {
    evaluatePoints(clipArray, isoValue, this->BatchSize, this);
  }
  const TInputIdType numberOfKeptPoints = evaluatePoints.NumberOfKeptPoints;
  const TableBasedPointBatches& pointBatches = evaluatePoints.PointBatches;
  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> pointsMap = evaluatePoints.PointsMap;
  // check if there are no kept points
  if (numberOfKeptPoints == 0)
  {
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  // Evaluate cells and calculate connectivitySize, numberOfOutputCells, numberOfCentroids,
  // cellBatches, cellsCase, edges
  EvaluateCellsWorker<TGrid, TInputIdType, TInsideOut> evaluateCells;
  if (!ScalarsDispatcher::Execute(clipArray, evaluateCells, input, isoValue, this->BatchSize, this))
  {
    evaluateCells(clipArray, input, isoValue, this->BatchSize, this);
  }
  const vtkIdType connectivitySize = evaluateCells.ConnectivitySize;
  const vtkIdType numberOfOutputCells = evaluateCells.NumberOfOutputCells;
  const vtkIdType faceConnectivitySize = evaluateCells.FacesConnectivitySize;
  const vtkIdType numberOfOutputFaces = evaluateCells.NumberOfOutputFaces;
  const vtkIdType numberOfCentroids = evaluateCells.NumberOfCentroids;
  const TableBasedCellBatches& cellBatches = evaluateCells.CellBatches;
  vtkSmartPointer<vtkUnsignedCharArray> cellsCase = evaluateCells.CellsCase;
  std::vector<EdgeType> edges = std::move(evaluateCells.Edges);
  std::vector<vtkIdType> unsupportedCells = std::move(evaluateCells.UnsupportedCells);

  // Create Edge locator which will be used to define the connectivity of cells
  EdgeLocatorType edgeLocator;
  if (!edges.empty())
  {
    edgeLocator.BuildLocator(static_cast<vtkIdType>(edges.size()), edges.data());
  }
  const vtkIdType numberOfEdges = edgeLocator.GetNumberOfEdges();

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
  vtkSmartPointer<vtkCellArray> outputFaces = nullptr;
  vtkSmartPointer<vtkCellArray> outputFaceLocations = nullptr;
  // initialize outputCellData
  auto outputCellData = vtkSmartPointer<vtkCellData>::New();
  ArrayList cellDataArrays;
  outputCellData->CopyAllocate(input->GetCellData(), numberOfOutputCells);
  cellDataArrays.AddArrays(numberOfOutputCells, input->GetCellData(), outputCellData,
    /*nullValue*/ 0.0, /*promote*/ false);

  // identify the required output id type
  bool createPolyhedrons = numberOfOutputFaces > 0;
  std::vector<Centroid> centroids;
#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds =
    (numberOfOutputPoints > VTK_TYPE_INT32_MAX || connectivitySize > VTK_TYPE_INT32_MAX ||
      numberOfOutputFaces > VTK_TYPE_INT32_MAX || faceConnectivitySize > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    // Extract cells and calculate centroids, types, cell array, cell data
    if (!createPolyhedrons)
    {
      ExtractCells<TGrid, TInputIdType, TOutputIdType, TInsideOut, false> extractCells(input,
        clipArray, pointsMap.Get(), cellsCase.Get(), cellBatches, cellDataArrays, edgeLocator,
        connectivitySize, numberOfOutputCells, faceConnectivitySize, numberOfOutputFaces,
        numberOfKeptPoints, numberOfEdges, numberOfCentroids, this, isoValue);
      vtkSMPTools::For(0, extractCells.CellBatches.GetNumberOfBatches(), extractCells);
      centroids = std::move(extractCells.Centroids);
      outputCellTypes = extractCells.OutputCellTypes;
      outputCellArray = extractCells.OutputCellArray;
    }
    else
    {
      ExtractCells<TGrid, TInputIdType, TOutputIdType, TInsideOut, true> extractCells(input,
        clipArray, pointsMap.Get(), cellsCase.Get(), cellBatches, cellDataArrays, edgeLocator,
        connectivitySize, numberOfOutputCells, faceConnectivitySize, numberOfOutputFaces,
        numberOfKeptPoints, numberOfEdges, numberOfCentroids, this, isoValue);
      vtkSMPTools::For(0, extractCells.CellBatches.GetNumberOfBatches(), extractCells);
      centroids = std::move(extractCells.Centroids);
      outputCellTypes = extractCells.OutputCellTypes;
      outputCellArray = extractCells.OutputCellArray;
      outputFaceLocations = extractCells.OutputFaceLocations;
      outputFaces = extractCells.OutputFaces;
    }
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    // Extract cells and calculate centroids, types, cell array, cell data
    if (!createPolyhedrons)
    {
      ExtractCells<TGrid, TInputIdType, TOutputIdType, TInsideOut, false> extractCells(input,
        clipArray, pointsMap.Get(), cellsCase.Get(), cellBatches, cellDataArrays, edgeLocator,
        connectivitySize, numberOfOutputCells, faceConnectivitySize, numberOfOutputFaces,
        numberOfKeptPoints, numberOfEdges, numberOfCentroids, this, isoValue);
      vtkSMPTools::For(0, extractCells.CellBatches.GetNumberOfBatches(), extractCells);
      centroids = std::move(extractCells.Centroids);
      outputCellTypes = extractCells.OutputCellTypes;
      outputCellArray = extractCells.OutputCellArray;
    }
    else
    {
      ExtractCells<TGrid, TInputIdType, TOutputIdType, TInsideOut, true> extractCells(input,
        clipArray, pointsMap.Get(), cellsCase.Get(), cellBatches, cellDataArrays, edgeLocator,
        connectivitySize, numberOfOutputCells, faceConnectivitySize, numberOfOutputFaces,
        numberOfKeptPoints, numberOfEdges, numberOfCentroids, this, isoValue);
      vtkSMPTools::For(0, extractCells.CellBatches.GetNumberOfBatches(), extractCells);
      centroids = std::move(extractCells.Centroids);
      outputCellTypes = extractCells.OutputCellTypes;
      outputCellArray = extractCells.OutputCellArray;
      outputFaceLocations = extractCells.OutputFaceLocations;
      outputFaces = extractCells.OutputFaces;
    }
  }
  // Extract points and calculate outputPoints and outputPointData.
  ExtractPointsWorker<TInputIdType> extractPointsWorker;
  using ExtractPointsDispatcher =
    vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::AllPointArrays,
      vtkArrayDispatch::AOSPointArrays>;
  if (!ExtractPointsDispatcher::Execute(inputPoints->GetData(), outputPoints->GetData(),
        extractPointsWorker, pointBatches, pointsMap.Get(), pointDataArrays, edges, centroids,
        numberOfKeptPoints, numberOfEdges, numberOfCentroids, this))
  {
    extractPointsWorker(inputPoints->GetData(), outputPoints->GetData(), pointBatches,
      pointsMap.Get(), pointDataArrays, edges, centroids, numberOfKeptPoints, numberOfEdges,
      numberOfCentroids, this);
  }
  if (this->GetGenerateClipPointTypes())
  {
    vtkNew<vtkUnsignedCharArray> clipPointTypes;
    clipPointTypes->SetName("vtkClipPointTypes");
    clipPointTypes->SetNumberOfTuples(outputPoints->GetNumberOfPoints());
    auto clipPointTypePtr = clipPointTypes->GetPointer(0);
    // Mark kept points
    vtkSMPTools::Fill(
      clipPointTypePtr, clipPointTypePtr + numberOfKeptPoints, static_cast<unsigned char>(0));
    // Mark edge points
    vtkSMPTools::Fill(clipPointTypePtr + numberOfKeptPoints,
      clipPointTypePtr + numberOfKeptPoints + numberOfEdges, static_cast<unsigned char>(1));
    // Mark centroid points
    vtkSMPTools::Fill(clipPointTypePtr + numberOfKeptPoints + numberOfEdges,
      clipPointTypePtr + outputPoints->GetNumberOfPoints(), static_cast<unsigned char>(2));
    outputPointData->AddArray(clipPointTypes);
  }

  // create outputClippedCells
  auto outputClippedCells = vtkSmartPointer<vtkUnstructuredGrid>::New();
  // if the input had no cells or it had cells but they were not all discarded, set points
  if (input->GetNumberOfCells() == 0 || outputCellArray->GetNumberOfCells() != 0)
  {
    outputClippedCells->SetPoints(outputPoints);
    outputClippedCells->GetPointData()->ShallowCopy(outputPointData);
    if (!unsupportedCells.empty())
    {
      vtkWarningMacro("Output points used by cells not supported by vtkTableBasedClipDataSet will "
                      "appear twice. To avoid this, consider using vtkClipDataSet directly");
    }
  }
  outputClippedCells->SetPolyhedralCells(
    outputCellTypes, outputCellArray, outputFaceLocations, outputFaces);
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
void vtkTableBasedClipDataSet::ClipPolyData(
  vtkPolyData* inputGrid, vtkDataArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG)
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
    this->ClipTDataSet(uGrid, scalars, isoValue, outputUG);
  }
  else
  {
    this->ClipTDataSet(inputGrid, scalars, isoValue, outputUG);
  }
}

//------------------------------------------------------------------------------
template <class TGrid>
void vtkTableBasedClipDataSet::ClipTDataSet(
  TGrid* inputGrid, vtkDataArray* scalars, double isoValue, vtkUnstructuredGrid* outputUG)
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
        clippedOutput = this->ClipTDataSet<TGrid, TInputIdType, true>(inputGrid, scalars, isoValue);
      }
      else
      {
        clippedOutput =
          this->ClipTDataSet<TGrid, TInputIdType, false>(inputGrid, scalars, isoValue);
      }
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      if (this->InsideOut)
      {
        clippedOutput = this->ClipTDataSet<TGrid, TInputIdType, true>(inputGrid, scalars, isoValue);
      }
      else
      {
        clippedOutput =
          this->ClipTDataSet<TGrid, TInputIdType, false>(inputGrid, scalars, isoValue);
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
