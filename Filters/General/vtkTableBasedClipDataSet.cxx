/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableBasedClipDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
 *
 * Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
 * Produced at the Lawrence Livermore National Laboratory
 * LLNL-CODE-400124
 * All rights reserved.
 *
 * This file was adapted from the VisIt clipper (vtkVisItClipper). For  details,
 * see https://visit.llnl.gov/.  The full copyright notice is contained in the
 * file COPYRIGHT located at the root of the VisIt distribution or at
 * http://www.llnl.gov/visit/copyright.html.
 *
 *****************************************************************************/

#include "vtkTableBasedClipDataSet.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkClipDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "vtkTableBasedClipCases.cxx"

vtkStandardNewMacro(vtkTableBasedClipDataSet);
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
  vtkUnstructuredGrid* output2 = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
  output2 = nullptr;

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

  if (this->AbortExecute)
  {
    algorithm->SetAbortExecute(1);
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
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkSmartPointer<vtkDataSet> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->CopyStructure(input);
  inputCopy->GetCellData()->PassData(input->GetCellData());
  inputCopy->GetFieldData()->PassData(input->GetFieldData());
  inputCopy->GetPointData()->InterpolateAllocate(input->GetPointData(), 0, 0, 1);

  // get the output (the remaining and the clipped parts)
  vtkUnstructuredGrid* outputUG =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* clippedOutputUG = this->GetClippedOutput();

  vtkDebugMacro(<< "Clipping dataset" << endl);

  vtkIdType numPoints = inputCopy->GetNumberOfPoints();

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
    // This is needed by vtkClipDataSet in case we fall back to it.
    inputCopy->GetPointData()->SetScalars(inputArray);
    if (!inputArray)
    {
      vtkErrorMacro(<< "no input scalars." << endl);
      return 1;
    }
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
      vtkSMPTools::For(0, numPoints, [&](vtkIdType begin, vtkIdType end) {
        for (vtkIdType i = begin; i < end; i++)
        {
          scalars->SetValue(i, inputArray->GetComponent(i, 0));
        }
      });
    }
  }

  double isoValue = (!this->ClipFunction || this->UseValueAsOffset) ? this->Value : 0.0;
  if (vtkImageData::SafeDownCast(inputCopy))
  {
    this->ClipImageData(inputCopy, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipImageData(inputCopy, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (vtkPolyData::SafeDownCast(inputCopy))
  {
    this->ClipPolyData(inputCopy, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipPolyData(inputCopy, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (vtkRectilinearGrid::SafeDownCast(inputCopy))
  {
    this->ClipRectilinearGrid(inputCopy, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipRectilinearGrid(inputCopy, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (vtkStructuredGrid::SafeDownCast(inputCopy))
  {
    this->ClipStructuredGrid(inputCopy, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipStructuredGrid(inputCopy, this->ClipFunction, scalars, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (vtkUnstructuredGridBase::SafeDownCast(inputCopy))
  {
    this->ClipUnstructuredGrid(inputCopy, this->ClipFunction, scalars, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipUnstructuredGrid(inputCopy, this->ClipFunction, scalars, isoValue, clippedOutputUG);
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
void vtkTableBasedClipDataSet::ClipDataSet(vtkDataSet* pDataSet, vtkUnstructuredGrid* outputUG)
{
  vtkNew<vtkClipDataSet> clipData;
  clipData->SetInputData(pDataSet);
  clipData->SetValue(this->Value);
  clipData->SetInsideOut(this->InsideOut);
  clipData->SetClipFunction(this->ClipFunction);
  clipData->SetUseValueAsOffset(this->UseValueAsOffset);
  clipData->SetGenerateClipScalars(this->GenerateClipScalars);
  clipData->Update();
  outputUG->ShallowCopy(clipData->GetOutput());
}

namespace // begin anonymous namespace
{
//------------------------------------------------------------------------------
// Extract the clipped cells is a 4-step process
// 1) Determine which input points will be kept and calculate Evaluate points
//    and calculate numberOfKeptPoints, pointsMap, clipArray.
//    1) Step 1 can be executed either with an implicit function or with scalars
// 2) Evaluate the input cells and calculate connectivitySize, numberOfOutputCells
//    numberOfCentroids, batchInfo, cellsMap, edges
// 3) Extract cells and calculate centroids, types, cell array, cell data
// 4) Extract points and point data

//-----------------------------------------------------------------------------
// Evaluate the implicit function equation for each input point.
// Develop a point map from the input points to output points.
template <typename TP, typename TInputIdType>
struct EvaluatePointsWithImplicitFunction
{
  TP* Points;
  vtkImplicitFunction* ImplicitFunction;
  double IsoValue;
  bool InsideOut;

  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> PointsMap;
  vtkSmartPointer<vtkDoubleArray> ClipArray;
  TInputIdType NumberOfKeptPoints;

  EvaluatePointsWithImplicitFunction(
    TP* points, vtkImplicitFunction* implicitFunction, double isoValue, bool insideOut)
    : Points(points)
    , ImplicitFunction(implicitFunction)
    , IsoValue(isoValue)
    , InsideOut(insideOut)
    , NumberOfKeptPoints(0)
  {
    const vtkIdType numberOfPoints = points->GetNumberOfTuples();
    this->PointsMap = vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>>::New();
    this->PointsMap->SetNumberOfValues(numberOfPoints);
    this->ClipArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->ClipArray->SetName("ClipDataSetScalars");
    this->ClipArray->SetNumberOfValues(numberOfPoints);
  }

  void Initialize() {}

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    const auto& points = vtk::DataArrayTupleRange<3>(this->Points, beginPointId, endPointId);
    auto pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap, beginPointId, endPointId);
    auto clipArray = vtk::DataArrayValueRange<1>(this->ClipArray, beginPointId, endPointId);

    double pointCopy[3];
    auto clipArrayIter = clipArray.begin();
    auto pointsMapIter = pointsMap.begin();
    for (const auto& point : points)
    {
      pointCopy[0] = point[0];
      pointCopy[1] = point[1];
      pointCopy[2] = point[2];

      // Outside points are marked with number < 0.
      *clipArrayIter = this->ImplicitFunction->FunctionValue(pointCopy);
      *pointsMapIter++ = this->InsideOut ? (*clipArrayIter++ - this->IsoValue >= 0.0 ? -1 : 1)
                                         : (*clipArrayIter++ - this->IsoValue >= 0.0 ? 1 : -1);
    }
  }

  void Reduce()
  {
    // Prefix sum to create point map of kept (i.e., retained) points.
    this->NumberOfKeptPoints = 0;
    for (auto& pointId : vtk::DataArrayValueRange<1>(this->PointsMap))
    {
      if (pointId > 0)
      {
        pointId = this->NumberOfKeptPoints++;
      }
    }
  }
};

//-----------------------------------------------------------------------------
// Develop a point map from the input points to output points using a scalar array.
template <typename TInputIdType>
struct EvaluatePointsWithScalarArray
{
  vtkDoubleArray* Scalars;
  double IsoValue;
  bool InsideOut;

  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> PointsMap;
  TInputIdType NumberOfKeptPoints;

  EvaluatePointsWithScalarArray(vtkDoubleArray* scalars, double isoValue, bool insideOut)
    : Scalars(scalars)
    , IsoValue(isoValue)
    , InsideOut(insideOut)
    , NumberOfKeptPoints(0)
  {
    this->PointsMap = vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>>::New();
    this->PointsMap->SetNumberOfValues(scalars->GetNumberOfTuples());
  }

  void Initialize() {}

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    const auto& scalars = vtk::DataArrayValueRange<1>(this->Scalars, beginPointId, endPointId);
    auto pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap, beginPointId, endPointId);

    auto pointsMapIter = pointsMap.begin();
    for (const auto& scalar : scalars)
    {
      // Outside points are marked with number < 0.
      *pointsMapIter++ = this->InsideOut ? (scalar - this->IsoValue >= 0.0 ? -1 : 1)
                                         : (scalar - this->IsoValue >= 0.0 ? 1 : -1);
    }
  }

  void Reduce()
  {
    // Prefix sum to create point map of kept (i.e., retained) points.
    this->NumberOfKeptPoints = 0;
    for (auto& pointId : vtk::DataArrayValueRange<1>(this->PointsMap))
    {
      if (pointId > 0)
      {
        pointId = this->NumberOfKeptPoints++;
      }
    }
  }
};

//-----------------------------------------------------------------------------
// Worker to evaluate Points both with scalars and implicit function.
template <typename TInputIdType>
struct EvaluatePointsWorker
{
  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> PointsMap;
  TInputIdType NumberOfKeptPoints;
  vtkSmartPointer<vtkDoubleArray> ClipArray;

  template <typename TPointsType>
  void operator()(
    TPointsType* pts, vtkImplicitFunction* implicitFunction, double isoValue, bool insideOut)
  {
    EvaluatePointsWithImplicitFunction<TPointsType, TInputIdType> evaluatePoints(
      pts, implicitFunction, isoValue, insideOut);
    vtkSMPTools::For(0, pts->GetNumberOfTuples(), evaluatePoints);
    this->NumberOfKeptPoints = evaluatePoints.NumberOfKeptPoints;
    this->PointsMap = evaluatePoints.PointsMap;
    this->ClipArray = evaluatePoints.ClipArray;
  }

  void operator()(vtkDoubleArray* scalars, double isoValue, bool insideOut)
  {
    EvaluatePointsWithScalarArray<TInputIdType> evaluatePoints(scalars, isoValue, insideOut);
    vtkSMPTools::For(0, scalars->GetNumberOfTuples(), evaluatePoints);
    this->NumberOfKeptPoints = evaluatePoints.NumberOfKeptPoints;
    this->PointsMap = evaluatePoints.PointsMap;
    this->ClipArray = evaluatePoints.Scalars;
  }
};

// 8 because of hexahedron.
#define MAX_CELL_SIZE 8

typedef int8_t EDGEIDXS[2];

//-----------------------------------------------------------------------------
// Keep track of output information within each batch of cells - this
// information is eventually rolled up into offsets into the cell
// connectivity and offsets arrays so that separate threads know where to
// write their data. We need to know how many total cells are created, the
// number of lines generated (which is equal to the number of clipped cells),
// and the connectivity size of the output cells and lines.
struct TableBasedBatch
{
  // These are accumulated in EvaluateCells::operator().
  vtkIdType NumberOfCells;
  vtkIdType NumberOfCentroids;
  vtkIdType CellsConnectivitySize;
  // These are needed because TableBasedBatchInfo will preserve only the batches
  // with NumberOfCells > 0
  vtkIdType BeginCellId;
  vtkIdType EndCellId;

  // These are assigned via prefix sum in EvaluateCells::Reduce(). This
  // information is used to instantiate the output cell arrays,
  vtkIdType BeginCellsOffsets;
  vtkIdType BeginCellsConnectivity;
  vtkIdType BeginCentroid;

  TableBasedBatch()
    : NumberOfCells(0)
    , NumberOfCentroids(0)
    , CellsConnectivitySize(0)
    , BeginCellId(0)
    , EndCellId(0)
    , BeginCellsOffsets(0)
    , BeginCellsConnectivity(0)
    , BeginCentroid(0)
  {
  }
};

//-----------------------------------------------------------------------------
struct TableBasedBatchInfo
{
  unsigned int BatchSize;
  std::vector<TableBasedBatch> Batches;
};

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
// numberOfCentroids, batchInfo, cellsMap, edges
template <typename TGrid, typename TInputIdType>
struct EvaluateCellsUnstructured
{
  using TEdge = EdgeType<TInputIdType>;

  TGrid* Input;
  vtkDoubleArray* ClipArray;
  double IsoValue;
  bool InsideOut;
  vtkIdType NumberOfInputCells;

  vtkSMPThreadLocalObject<vtkIdList> TLIdList;
  vtkSMPThreadLocal<std::vector<TEdge>> TLEdges;

  TableBasedBatchInfo BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> CellsMap;
  std::vector<TEdge> Edges;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfCentroids;

  EvaluateCellsUnstructured(TGrid* input, vtkDoubleArray* clipArray, double isoValue,
    bool insideOut, unsigned int batchSize)
    : Input(input)
    , ClipArray(clipArray)
    , IsoValue(isoValue)
    , InsideOut(insideOut)
    , NumberOfInputCells(input->GetNumberOfCells())
    , ConnectivitySize(0)
    , NumberOfOutputCells(0)
    , NumberOfCentroids(0)
  {
    // initialize batches
    this->BatchInfo.BatchSize = batchSize;
    size_t numberOfBatches = static_cast<size_t>(((this->NumberOfInputCells - 1) / batchSize) + 1);
    this->BatchInfo.Batches.resize(numberOfBatches);
    // initialize cellsMap
    this->CellsMap = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->CellsMap->SetNumberOfValues(this->NumberOfInputCells);
    // build cells for polydata so that you can use GetCellPoints()
    vtkNew<vtkGenericCell> cell;
    this->Input->GetCell(0, cell);
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
    const auto& clipArray = vtk::DataArrayValueRange<1>(this->ClipArray);
    auto cellsMap = vtk::DataArrayValueRange<1>(this->CellsMap);
    const vtkIdType* pointIndices;
    vtkIdType numberOfPoints, j, cellId, batchSize, numberOfCells, numberOfCentroids,
      cellsConnectivitySize;
    TInputIdType pointIndex1, pointIndex2;
    int caseIndex, cellType;
    int16_t color;
    double grdDiffs[8], point1ToPoint2, point1ToIso, point1Weight;
    uint16_t startIndex;
    uint8_t numberOfOutputs, shape, numberOfCellPoints, p, pointIndex, point1Index, point2Index;
    uint8_t* thisCase = nullptr;
    const EDGEIDXS* edgeVertices = nullptr;

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      TableBasedBatch& batch = this->BatchInfo.Batches[batchId];
      batchSize = static_cast<vtkIdType>(this->BatchInfo.BatchSize);
      batch.BeginCellId = batchId * batchSize;
      batch.EndCellId =
        (batch.BeginCellId + batchSize > this->NumberOfInputCells ? this->NumberOfInputCells
                                                                  : batch.BeginCellId + batchSize);
      for (cellId = batch.BeginCellId; cellId < batch.EndCellId; ++cellId)
      {
        cellType = this->Input->GetCellType(cellId);
        this->Input->GetCellPoints(cellId, numberOfPoints, pointIndices, idList);

        caseIndex = 0;
        for (j = numberOfPoints - 1; j >= 0; --j)
        {
          grdDiffs[j] = clipArray[pointIndices[j]] - this->IsoValue;
          caseIndex += ((grdDiffs[j] >= 0.0) ? 1 : 0);
          caseIndex <<= (1 - (!j));
        }

        // start index, split case, number of output, and vertices from edges
        numberOfOutputs = 0;
        switch (cellType)
        {
          case VTK_VOXEL:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesVox[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesVox[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesVox[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::VoxVerticesFromEdges;
            break;

          case VTK_HEXAHEDRON:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesHex[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
            break;

          case VTK_WEDGE:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesWdg[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesWdg[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesWdg[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
            break;

          case VTK_PYRAMID:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesPyr[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesPyr[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesPyr[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
            break;

          case VTK_TETRA:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesTet[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesTet[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesTet[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
            break;

          case VTK_PIXEL:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesPix[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesPix[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesPix[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::PixelVerticesFromEdges;
            break;

          case VTK_QUAD:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesQua[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
            break;

          case VTK_TRIANGLE:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesTri[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesTri[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesTri[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
            break;

          case VTK_LINE:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesLin[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesLin[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesLin[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
            break;

          case VTK_VERTEX:
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesVtx[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesVtx[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesVtx[caseIndex];
            edgeVertices = nullptr;
            break;
        }

        numberOfCells = 0;
        cellsConnectivitySize = 0;
        numberOfCentroids = 0;
        for (j = 0; j < numberOfOutputs; j++)
        {
          numberOfCellPoints = 0;
          color = -1;
          shape = *thisCase++;
          // number of points and color
          switch (shape)
          {
            case ST_HEX:
              numberOfCellPoints = 8;
              color = *thisCase++;
              break;

            case ST_WDG:
              numberOfCellPoints = 6;
              color = *thisCase++;
              break;

            case ST_PYR:
              numberOfCellPoints = 5;
              color = *thisCase++;
              break;

            case ST_TET:
              numberOfCellPoints = 4;
              color = *thisCase++;
              break;

            case ST_QUA:
              numberOfCellPoints = 4;
              color = *thisCase++;
              break;

            case ST_TRI:
              numberOfCellPoints = 3;
              color = *thisCase++;
              break;

            case ST_LIN:
              numberOfCellPoints = 2;
              color = *thisCase++;
              break;

            case ST_VTX:
              numberOfCellPoints = 1;
              color = *thisCase++;
              break;

            case ST_PNT:
              thisCase++;
              color = *thisCase++;
              numberOfCellPoints = *thisCase++;
              break;

            default:
              vtkLogF(ERROR, "An invalid output shape was found in the ClipCases.");
          }

          if ((!this->InsideOut && color == COLOR0) || (this->InsideOut && color == COLOR1))
          {
            // We don't want this one; it's the wrong side.
            thisCase += numberOfCellPoints;
            continue;
          }
          for (p = 0; p < numberOfCellPoints; ++p)
          {
            pointIndex = *thisCase++;

            if (pointIndex > P7 && pointIndex >= EA && pointIndex <= EL) // Mid-Edge Point
            {
              point1Index = edgeVertices[pointIndex - EA][0];
              point2Index = edgeVertices[pointIndex - EA][1];
              if (point2Index < point1Index)
              {
                std::swap(point1Index, point2Index);
              }
              point1ToPoint2 = grdDiffs[point2Index] - grdDiffs[point1Index];
              point1ToIso = 0.0 - grdDiffs[point1Index];
              point1Weight = 1.0 - point1ToIso / point1ToPoint2;

              pointIndex1 = static_cast<TInputIdType>(pointIndices[point1Index]);
              pointIndex2 = static_cast<TInputIdType>(pointIndices[point2Index]);

              // swap in case the order is wrong
              if (pointIndex1 > pointIndex2)
              {
                std::swap(pointIndex1, pointIndex2);
                point1Weight = 1.0 - point1Weight;
              }
              edges.emplace_back(pointIndex1, pointIndex2, point1Weight);
            }
          }
          switch (shape)
          {
            case ST_HEX:
              numberOfCells++;
              cellsConnectivitySize += 8;
              break;

            case ST_WDG:
              numberOfCells++;
              cellsConnectivitySize += 6;
              break;

            case ST_PYR:
              numberOfCells++;
              cellsConnectivitySize += 5;
              break;

            case ST_TET:
              numberOfCells++;
              cellsConnectivitySize += 4;
              break;

            case ST_QUA:
              numberOfCells++;
              cellsConnectivitySize += 4;
              break;

            case ST_TRI:
              numberOfCells++;
              cellsConnectivitySize += 3;
              break;

            case ST_LIN:
              numberOfCells++;
              cellsConnectivitySize += 2;
              break;

            case ST_VTX:
              numberOfCells++;
              cellsConnectivitySize += 1;
              break;

            case ST_PNT:
              numberOfCentroids++;
              break;
          }
        }
        batch.NumberOfCells += numberOfCells;
        batch.NumberOfCentroids += numberOfCentroids;
        batch.CellsConnectivitySize += cellsConnectivitySize;
        cellsMap[cellId] = numberOfCells > 0 ? 1 : 0;
      }
    }
  }

public:
  void Reduce()
  {
    this->ConnectivitySize = 0;
    this->NumberOfOutputCells = 0;
    this->NumberOfCentroids = 0;
    vtkIdType beginCellsOffsets = 0, beginCellsConnectivity = 0, beginCentroid = 0;

    // assign BeginCellsOffsets/BeginCellsConnectivity/BeginCentroid for each batch
    // and remove the batch with 0 cells (in-place)
    size_t batchWithOutputCellsIndex = 0;
    for (size_t i = 0; i < this->BatchInfo.Batches.size(); ++i)
    {
      auto& batch = this->BatchInfo.Batches[i];
      if (batch.NumberOfCells > 0)
      {
        batch.BeginCellsOffsets = beginCellsOffsets;
        batch.BeginCellsConnectivity = beginCellsConnectivity;
        batch.BeginCentroid = beginCentroid;

        beginCellsOffsets += batch.NumberOfCells;
        beginCellsConnectivity += batch.CellsConnectivitySize;
        beginCentroid += batch.NumberOfCentroids;

        this->NumberOfOutputCells += batch.NumberOfCells;
        this->NumberOfCentroids += batch.NumberOfCentroids;
        this->ConnectivitySize += batch.CellsConnectivitySize;
        if (i != batchWithOutputCellsIndex)
        {
          this->BatchInfo.Batches[batchWithOutputCellsIndex] = batch;
        }
        batchWithOutputCellsIndex++;
      }
    }
    this->BatchInfo.Batches.resize(batchWithOutputCellsIndex);

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
    vtkSMPTools::For(
      0, static_cast<vtkIdType>(tlEdgesVector.size()), [&](vtkIdType begin, vtkIdType end) {
        for (vtkIdType threadId = begin; threadId < end; ++threadId)
        {
          auto& edges = *tlEdgesVector[threadId];
          std::copy(edges.begin(), edges.end(), this->Edges.begin() + beginIndices[threadId]);
        }
      });
  }

  void Execute()
  {
    vtkSMPTools::For(0, static_cast<vtkIdType>(this->BatchInfo.Batches.size()), *this);
  }
};

//-----------------------------------------------------------------------------
// Evaluate structured cells and calculate connectivitySize, numberOfOutputCells,
// numberOfCentroids, batchInfo, cellsMap, edges
template <typename TGrid, typename TInputIdType>
struct EvaluateCellsStructured
{
  using TEdge = EdgeType<TInputIdType>;

  TGrid* Input;
  vtkDoubleArray* ClipArray;
  double IsoValue;
  bool InsideOut;
  vtkIdType NumberOfInputCells;

  enum TwoDimensionType
  {
    XY,
    YZ,
    XZ
  };
  TwoDimensionType TwoDimType;
  int IsTwoDim;

  std::array<std::array<int, 8>, 3> ShiftLUT;
  std::array<int, 3> CellDims;
  int CyStride;
  int CzStride;
  int PyStride;
  int PzStride;

  vtkSMPThreadLocal<std::vector<TEdge>> TLEdges;

  TableBasedBatchInfo BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> CellsMap;
  std::vector<TEdge> Edges;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfCentroids;

  EvaluateCellsStructured(TGrid* input, vtkDoubleArray* clipArray, double isoValue, bool insideOut,
    unsigned int batchSize)
    : Input(input)
    , ClipArray(clipArray)
    , IsoValue(isoValue)
    , InsideOut(insideOut)
    , NumberOfInputCells(input->GetNumberOfCells())
    , ConnectivitySize(0)
    , NumberOfOutputCells(0)
    , NumberOfCentroids(0)
  {
    // initialize batches
    this->BatchInfo.BatchSize = batchSize;
    size_t numberOfBatches = static_cast<size_t>(((this->NumberOfInputCells - 1) / batchSize) + 1);
    this->BatchInfo.Batches.resize(numberOfBatches);
    // initialize cellsMap
    this->CellsMap = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->CellsMap->SetNumberOfValues(this->NumberOfInputCells);

    int gridDims[3];
    this->Input->GetDimensions(gridDims);
    this->IsTwoDim = int(gridDims[0] <= 1 || gridDims[1] <= 1 || gridDims[2] <= 1);
    if (gridDims[0] <= 1)
    {
      this->TwoDimType = YZ;
    }
    else if (gridDims[1] <= 1)
    {
      this->TwoDimType = XZ;
    }
    else
    {
      this->TwoDimType = XY;
    }

    const std::array<int, 8> shiftLUTx = { 0, 1, 1, 0, 0, 1, 1, 0 };
    const std::array<int, 8> shiftLUTy = { 0, 0, 1, 1, 0, 0, 1, 1 };
    const std::array<int, 8> shiftLUTz = { 0, 0, 0, 0, 1, 1, 1, 1 };

    if (this->IsTwoDim && this->TwoDimType == XZ)
    {
      this->ShiftLUT[0] = shiftLUTx;
      this->ShiftLUT[1] = shiftLUTz;
      this->ShiftLUT[2] = shiftLUTy;
    }
    else if (this->IsTwoDim && this->TwoDimType == YZ)
    {
      this->ShiftLUT[0] = shiftLUTy;
      this->ShiftLUT[1] = shiftLUTz;
      this->ShiftLUT[2] = shiftLUTx;
    }
    else
    {
      this->ShiftLUT[0] = shiftLUTx;
      this->ShiftLUT[1] = shiftLUTy;
      this->ShiftLUT[2] = shiftLUTz;
    }

    this->CellDims = { gridDims[0] - 1, gridDims[1] - 1, gridDims[2] - 1 };
    this->CyStride = (this->CellDims[0] ? this->CellDims[0] : 1);
    this->CzStride =
      (this->CellDims[0] ? this->CellDims[0] : 1) * (this->CellDims[1] ? this->CellDims[1] : 1);
    this->PyStride = gridDims[0];
    this->PzStride = gridDims[0] * gridDims[1];
  }

  void Initialize()
  {
    // initialize edges
    this->TLEdges.Local().reserve(static_cast<size_t>(this->Input->GetNumberOfPoints() * 0.001));
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto& edges = this->TLEdges.Local();
    const auto& clipArray = vtk::DataArrayValueRange<1>(this->ClipArray);
    auto cellsMap = vtk::DataArrayValueRange<1>(this->CellsMap);
    double grdDiffs[8], point1ToPoint2, point1ToIso, point1Weight;
    vtkIdType cellPointIndex, j, cellId, batchSize, numberOfCells, numberOfCentroids,
      cellsConnectivitySize;
    TInputIdType pointIndex1, pointIndex2;
    int caseIndex, theCellI, theCellJ, theCellK;
    int16_t color;
    const int8_t numberOfPoints = this->IsTwoDim ? 4 : 8;
    uint16_t startIndex;
    uint8_t numberOfOutputs, shape, numberOfCellPoints, p, pointIndex, point1Index, point2Index;
    uint8_t* thisCase = nullptr;
    const EDGEIDXS* edgeVertices = nullptr;

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      TableBasedBatch& batch = this->BatchInfo.Batches[batchId];
      batchSize = static_cast<vtkIdType>(this->BatchInfo.BatchSize);
      batch.BeginCellId = batchId * batchSize;
      batch.EndCellId =
        (batch.BeginCellId + batchSize > this->NumberOfInputCells ? this->NumberOfInputCells
                                                                  : batch.BeginCellId + batchSize);
      for (cellId = batch.BeginCellId; cellId < batch.EndCellId; ++cellId)
      {
        theCellI = (this->CellDims[0] > 0 ? cellId % this->CellDims[0] : 0);
        theCellJ = (this->CellDims[1] > 0 ? (cellId / this->CyStride) % this->CellDims[1] : 0);
        theCellK = (this->CellDims[2] > 0 ? (cellId / this->CzStride) : 0);

        caseIndex = 0;
        for (j = numberOfPoints - 1; j >= 0; j--)
        {
          cellPointIndex = (theCellI + this->ShiftLUT[0][j]) +
            (theCellJ + this->ShiftLUT[1][j]) * this->PyStride +
            (theCellK + this->ShiftLUT[2][j]) * this->PzStride;

          grdDiffs[j] = clipArray[cellPointIndex] - this->IsoValue;
          caseIndex += ((grdDiffs[j] >= 0.0) ? 1 : 0);
          caseIndex <<= (1 - (!j));
        }

        // start index, split case, number of output, and vertices from edges
        if (this->IsTwoDim)
        {
          startIndex = vtkTableBasedClipperClipTables::StartClipShapesQua[caseIndex];
          thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua[startIndex];
          numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[caseIndex];
          edgeVertices = vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
        }
        else
        {
          startIndex = vtkTableBasedClipperClipTables::StartClipShapesHex[caseIndex];
          thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex[startIndex];
          numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[caseIndex];
          edgeVertices = vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
        }

        numberOfCells = 0;
        cellsConnectivitySize = 0;
        numberOfCentroids = 0;
        for (j = 0; j < numberOfOutputs; j++)
        {
          numberOfCellPoints = 0;
          color = -1;
          shape = *thisCase++;
          // number of points and color
          switch (shape)
          {
            case ST_HEX:
              numberOfCellPoints = 8;
              color = *thisCase++;
              break;

            case ST_WDG:
              numberOfCellPoints = 6;
              color = *thisCase++;
              break;

            case ST_PYR:
              numberOfCellPoints = 5;
              color = *thisCase++;
              break;

            case ST_TET:
              numberOfCellPoints = 4;
              color = *thisCase++;
              break;

            case ST_QUA:
              numberOfCellPoints = 4;
              color = *thisCase++;
              break;

            case ST_TRI:
              numberOfCellPoints = 3;
              color = *thisCase++;
              break;

            case ST_LIN:
              numberOfCellPoints = 2;
              color = *thisCase++;
              break;

            case ST_VTX:
              numberOfCellPoints = 1;
              color = *thisCase++;
              break;

            case ST_PNT:
              thisCase++;
              color = *thisCase++;
              numberOfCellPoints = *thisCase++;
              break;

            default:
              vtkLogF(ERROR, "An invalid output shape was found in the ClipCases.");
          }

          if ((!this->InsideOut && color == COLOR0) || (this->InsideOut && color == COLOR1))
          {
            // We don't want this one; it's the wrong side.
            thisCase += numberOfCellPoints;
            continue;
          }
          for (p = 0; p < numberOfCellPoints; ++p)
          {
            pointIndex = *thisCase++;

            if (pointIndex > P7 && pointIndex >= EA && pointIndex <= EL) // Mid-Edge Point
            {
              point1Index = edgeVertices[pointIndex - EA][0];
              point2Index = edgeVertices[pointIndex - EA][1];
              if (point2Index < point1Index)
              {
                std::swap(point1Index, point2Index);
              }
              point1ToPoint2 = grdDiffs[point2Index] - grdDiffs[point1Index];
              point1ToIso = 0.0 - grdDiffs[point1Index];
              point1Weight = 1.0 - point1ToIso / point1ToPoint2;

              pointIndex1 = ((theCellI + this->ShiftLUT[0][point1Index]) +
                (theCellJ + this->ShiftLUT[1][point1Index]) * this->PyStride +
                (theCellK + this->ShiftLUT[2][point1Index]) * this->PzStride);
              pointIndex2 = ((theCellI + this->ShiftLUT[0][point2Index]) +
                (theCellJ + this->ShiftLUT[1][point2Index]) * this->PyStride +
                (theCellK + this->ShiftLUT[2][point2Index]) * this->PzStride);

              // swap in case the order is wrong
              if (pointIndex1 > pointIndex2)
              {
                std::swap(pointIndex1, pointIndex2);
                point1Weight = 1.0 - point1Weight;
              }
              edges.emplace_back(pointIndex1, pointIndex2, point1Weight);
            }
          }
          switch (shape)
          {
            case ST_HEX:
              numberOfCells++;
              cellsConnectivitySize += 8;
              break;

            case ST_WDG:
              numberOfCells++;
              cellsConnectivitySize += 6;
              break;

            case ST_PYR:
              numberOfCells++;
              cellsConnectivitySize += 5;
              break;

            case ST_TET:
              numberOfCells++;
              cellsConnectivitySize += 4;
              break;

            case ST_QUA:
              numberOfCells++;
              cellsConnectivitySize += 4;
              break;

            case ST_TRI:
              numberOfCells++;
              cellsConnectivitySize += 3;
              break;

            case ST_LIN:
              numberOfCells++;
              cellsConnectivitySize += 2;
              break;

            case ST_VTX:
              numberOfCells++;
              cellsConnectivitySize += 1;
              break;

            case ST_PNT:
              numberOfCentroids++;
              break;
          }
        }
        batch.NumberOfCells += numberOfCells;
        batch.NumberOfCentroids += numberOfCentroids;
        batch.CellsConnectivitySize += cellsConnectivitySize;
        cellsMap[cellId] = numberOfCells > 0 ? 1 : 0;
      }
    }
  }

  void Reduce()
  {
    this->ConnectivitySize = 0;
    this->NumberOfOutputCells = 0;
    this->NumberOfCentroids = 0;
    vtkIdType beginCellsOffsets = 0, beginCellsConnectivity = 0, beginCentroid = 0;

    // assign BeginCellsOffsets/BeginCellsConnectivity/BeginCentroid for each batch
    // and remove the batch with 0 cells (in-place)
    size_t batchWithOutputCellsIndex = 0;
    for (size_t i = 0; i < this->BatchInfo.Batches.size(); ++i)
    {
      auto& batch = this->BatchInfo.Batches[i];
      if (batch.NumberOfCells > 0)
      {
        batch.BeginCellsOffsets = beginCellsOffsets;
        batch.BeginCellsConnectivity = beginCellsConnectivity;
        batch.BeginCentroid = beginCentroid;

        beginCellsOffsets += batch.NumberOfCells;
        beginCellsConnectivity += batch.CellsConnectivitySize;
        beginCentroid += batch.NumberOfCentroids;

        this->NumberOfOutputCells += batch.NumberOfCells;
        this->NumberOfCentroids += batch.NumberOfCentroids;
        this->ConnectivitySize += batch.CellsConnectivitySize;
        if (i != batchWithOutputCellsIndex)
        {
          this->BatchInfo.Batches[batchWithOutputCellsIndex] = batch;
        }
        batchWithOutputCellsIndex++;
      }
    }
    this->BatchInfo.Batches.resize(batchWithOutputCellsIndex);

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
    vtkSMPTools::For(
      0, static_cast<vtkIdType>(tlEdgesVector.size()), [&](vtkIdType begin, vtkIdType end) {
        for (vtkIdType threadId = begin; threadId < end; ++threadId)
        {
          auto& edges = *tlEdgesVector[threadId];
          std::copy(edges.begin(), edges.end(), this->Edges.begin() + beginIndices[threadId]);
        }
      });
  }

  void Execute()
  {
    vtkSMPTools::For(0, static_cast<vtkIdType>(this->BatchInfo.Batches.size()), *this);
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
  Centroid(const vtkIdType* pointIds, uint8_t numberOfPoints)
    : NumberOfPoints(numberOfPoints)
  {
    std::copy(pointIds, pointIds + numberOfPoints, this->PointIds);
  }
};

//-----------------------------------------------------------------------------
// Extract cells unstructured
template <typename TGrid, typename TInputIdType, typename TOutputIdType>
struct ExtractCellsUnstructured
{
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  using TOutputIdTypeArray = vtkAOSDataArrayTemplate<TOutputIdType>;

  TGrid* Input;
  vtkDoubleArray* ClipArray;
  double IsoValue;
  bool InsideOut;
  vtkAOSDataArrayTemplate<TInputIdType>* PointsMap;
  vtkUnsignedCharArray* CellsMap;
  const TableBasedBatchInfo& BatchInfo;
  ArrayList& CellDataArrays;
  const TEdgeLocator& EdgeLocator;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfKeptPoints;
  vtkIdType NumberOfEdges;
  vtkIdType NumberOfCentroids;
  vtkIdType NumberOfKeptPointsAndEdges;

  vtkSMPThreadLocalObject<vtkIdList> TLIdList;

  vtkSmartPointer<TOutputIdTypeArray> Connectivity;
  vtkSmartPointer<TOutputIdTypeArray> Offsets;

  std::vector<Centroid> Centroids;
  vtkSmartPointer<vtkUnsignedCharArray> OutputCellTypes;
  vtkSmartPointer<vtkCellArray> OutputCellArray;

  ExtractCellsUnstructured(TGrid* input, vtkDoubleArray* clipArray, double isoValue, bool insideOut,
    vtkAOSDataArrayTemplate<TInputIdType>* pointsMap, vtkUnsignedCharArray* cellsMap,
    const TableBasedBatchInfo& batchInfo, ArrayList& cellDataArrays,
    const TEdgeLocator& edgeLocator, vtkIdType connectivitySize, vtkIdType numberOfOutputCells,
    vtkIdType numberOfKeptPoints, vtkIdType numberOfEdges, vtkIdType numberOfCentroids)
    : Input(input)
    , ClipArray(clipArray)
    , IsoValue(isoValue)
    , InsideOut(insideOut)
    , PointsMap(pointsMap)
    , CellsMap(cellsMap)
    , BatchInfo(batchInfo)
    , CellDataArrays(cellDataArrays)
    , EdgeLocator(edgeLocator)
    , ConnectivitySize(connectivitySize)
    , NumberOfOutputCells(numberOfOutputCells)
    , NumberOfKeptPoints(numberOfKeptPoints)
    , NumberOfEdges(numberOfEdges)
    , NumberOfCentroids(numberOfCentroids)
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
    // set NumberOfKeptPointsAndEdges
    this->NumberOfKeptPointsAndEdges = this->NumberOfKeptPoints + this->NumberOfEdges;
  }

  void Initialize()
  {
    // initialize list size
    this->TLIdList.Local()->Allocate(MAX_CELL_SIZE);
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    vtkIdList*& idList = this->TLIdList.Local();
    const auto& clipArray = vtk::DataArrayValueRange<1>(this->ClipArray);
    const auto& pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap);
    const auto& cellsMap = vtk::DataArrayValueRange<1>(this->CellsMap);
    auto connectivity = vtk::DataArrayValueRange<1>(this->Connectivity);
    auto offsets = vtk::DataArrayValueRange<1>(this->Offsets);
    auto types = vtk::DataArrayValueRange<1>(this->OutputCellTypes);
    const vtkIdType* pointIndices;
    vtkIdType numberOfPoints, j, outputCellId, offset, outputCentroidId, cellId;
    vtkIdType centroidIds[4], shapeIds[MAX_CELL_SIZE];
    TInputIdType pointIndex1, pointIndex2;
    double grdDiffs[8];
    int caseIndex, cellType;
    int16_t centroidIndex, color;
    uint16_t startIndex;
    uint8_t numberOfOutputs, shape, numberOfCellPoints, p, pointIndex, point1Index, point2Index;
    uint8_t* thisCase = nullptr;
    const EDGEIDXS* edgeVertices = nullptr;

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      const TableBasedBatch& batch = this->BatchInfo.Batches[batchId];
      outputCellId = batch.BeginCellsOffsets;
      offset = batch.BeginCellsConnectivity;
      outputCentroidId = batch.BeginCentroid;

      for (cellId = batch.BeginCellId; cellId < batch.EndCellId; ++cellId)
      {
        // process cells that has output cells (either itself or at least because it's clipped)
        if (cellsMap[cellId] == 1)
        {
          this->Input->GetCellPoints(cellId, numberOfPoints, pointIndices, idList);

          caseIndex = 0;
          for (j = numberOfPoints - 1; j >= 0; --j)
          {
            grdDiffs[j] = clipArray[pointIndices[j]] - this->IsoValue;
            caseIndex += ((grdDiffs[j] >= 0.0) ? 1 : 0);
            caseIndex <<= (1 - (!j));
          }

          // start index, split case, number of output, and vertices from edges
          cellType = this->Input->GetCellType(cellId);
          numberOfOutputs = 0;
          switch (cellType)
          {
            case VTK_VOXEL:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesVox[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesVox[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesVox[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::VoxVerticesFromEdges;
              break;

            case VTK_HEXAHEDRON:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesHex[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
              break;

            case VTK_WEDGE:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesWdg[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesWdg[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesWdg[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
              break;

            case VTK_PYRAMID:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesPyr[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesPyr[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesPyr[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
              break;

            case VTK_TETRA:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesTet[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesTet[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesTet[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
              break;

            case VTK_PIXEL:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesPix[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesPix[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesPix[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::PixelVerticesFromEdges;
              break;

            case VTK_QUAD:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesQua[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
              break;

            case VTK_TRIANGLE:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesTri[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesTri[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesTri[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
              break;

            case VTK_LINE:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesLin[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesLin[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesLin[caseIndex];
              edgeVertices = vtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
              break;

            case VTK_VERTEX:
              startIndex = vtkTableBasedClipperClipTables::StartClipShapesVtx[caseIndex];
              thisCase = &vtkTableBasedClipperClipTables::ClipShapesVtx[startIndex];
              numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesVtx[caseIndex];
              edgeVertices = nullptr;
              break;
          }

          for (j = 0; j < numberOfOutputs; j++)
          {
            numberOfCellPoints = 0;
            color = -1;
            centroidIndex = -1;
            shape = *thisCase++;
            // number of points and color
            switch (shape)
            {
              case ST_HEX:
                numberOfCellPoints = 8;
                color = *thisCase++;
                break;

              case ST_WDG:
                numberOfCellPoints = 6;
                color = *thisCase++;
                break;

              case ST_PYR:
                numberOfCellPoints = 5;
                color = *thisCase++;
                break;

              case ST_TET:
                numberOfCellPoints = 4;
                color = *thisCase++;
                break;

              case ST_QUA:
                numberOfCellPoints = 4;
                color = *thisCase++;
                break;

              case ST_TRI:
                numberOfCellPoints = 3;
                color = *thisCase++;
                break;

              case ST_LIN:
                numberOfCellPoints = 2;
                color = *thisCase++;
                break;

              case ST_VTX:
                numberOfCellPoints = 1;
                color = *thisCase++;
                break;

              case ST_PNT:
                centroidIndex = *thisCase++;
                color = *thisCase++;
                numberOfCellPoints = *thisCase++;
                break;

              default:
                vtkLogF(ERROR, "An invalid output shape was found in the ClipCases.");
            }

            if ((!this->InsideOut && color == COLOR0) || (this->InsideOut && color == COLOR1))
            {
              // We don't want this one; it's the wrong side.
              thisCase += numberOfCellPoints;
              continue;
            }
            for (p = 0; p < numberOfCellPoints; ++p)
            {
              pointIndex = *thisCase++;

              if (pointIndex <= P7) // Input Point
              {
                // We know pt P0 must be > P0 since we already
                // assume P0 == 0.  This is why we do not
                // bother subtracting P0 from pt here.
                shapeIds[p] = pointsMap[pointIndices[pointIndex]];
              }
              else if (pointIndex >= EA && pointIndex <= EL) // Mid-Edge Point
              {
                point1Index = edgeVertices[pointIndex - EA][0];
                point2Index = edgeVertices[pointIndex - EA][1];
                if (point2Index < point1Index)
                {
                  std::swap(point1Index, point2Index);
                }

                pointIndex1 = static_cast<TInputIdType>(pointIndices[point1Index]);
                pointIndex2 = static_cast<TInputIdType>(pointIndices[point2Index]);

                shapeIds[p] = this->NumberOfKeptPoints +
                  this->EdgeLocator.IsInsertedEdge(pointIndex1, pointIndex2);
              }
              else if (pointIndex >= N0 && pointIndex <= N3) // Centroid Point
              {
                shapeIds[p] = centroidIds[pointIndex - N0];
              }
              else
              {
                vtkLogF(ERROR, "An invalid output shape was found in the ClipCases.");
              }
            }

            switch (shape)
            {
              case ST_HEX:
                types[outputCellId] = VTK_HEXAHEDRON;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 8; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_WDG:
                types[outputCellId] = VTK_WEDGE;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 6; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_PYR:
                types[outputCellId] = VTK_PYRAMID;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 5; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_TET:
                types[outputCellId] = VTK_TETRA;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 4; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_QUA:
                types[outputCellId] = VTK_QUAD;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 4; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_TRI:
                types[outputCellId] = VTK_TRIANGLE;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 3; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_LIN:
                types[outputCellId] = VTK_LINE;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 2; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_VTX:
                types[outputCellId] = VTK_VERTEX;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[0]);
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_PNT:
                this->Centroids[outputCentroidId] = Centroid(shapeIds, numberOfCellPoints);
                centroidIds[centroidIndex] = this->NumberOfKeptPointsAndEdges + outputCentroidId++;
            }
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

  void Execute()
  {
    vtkSMPTools::For(0, static_cast<vtkIdType>(this->BatchInfo.Batches.size()), *this);
  }
};

//-----------------------------------------------------------------------------
// Extract cells structured
template <typename TGrid, typename TInputIdType, typename TOutputIdType>
struct ExtractCellsStructured
{
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  using TOutputIdTypeArray = vtkAOSDataArrayTemplate<TOutputIdType>;

  TGrid* Input;
  vtkDoubleArray* ClipArray;
  double IsoValue;
  bool InsideOut;
  vtkAOSDataArrayTemplate<TInputIdType>* PointsMap;
  vtkUnsignedCharArray* CellsMap;
  const TableBasedBatchInfo& BatchInfo;
  ArrayList& CellDataArrays;
  const TEdgeLocator& EdgeLocator;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfKeptPoints;
  vtkIdType NumberOfEdges;
  vtkIdType NumberOfCentroids;
  vtkIdType NumberOfKeptPointsAndEdges;

  enum TwoDimensionType
  {
    XY,
    YZ,
    XZ
  };
  TwoDimensionType TwoDimType;
  int IsTwoDim;

  std::array<std::array<int, 8>, 3> ShiftLUT;
  std::array<int, 3> CellDims;
  int CyStride;
  int CzStride;
  int PyStride;
  int PzStride;

  vtkSmartPointer<TOutputIdTypeArray> Connectivity;
  vtkSmartPointer<TOutputIdTypeArray> Offsets;

  std::vector<Centroid> Centroids;
  vtkSmartPointer<vtkUnsignedCharArray> OutputCellTypes;
  vtkSmartPointer<vtkCellArray> OutputCellArray;

  ExtractCellsStructured(TGrid* input, vtkDoubleArray* clipArray, double isoValue, bool insideOut,
    vtkAOSDataArrayTemplate<TInputIdType>* pointsMap, vtkUnsignedCharArray* cellsMap,
    const TableBasedBatchInfo& batchInfo, ArrayList& cellDataArrays,
    const TEdgeLocator& edgeLocator, vtkIdType connectivitySize, vtkIdType numberOfOutputCells,
    vtkIdType numberOfKeptPoints, vtkIdType numberOfEdges, vtkIdType numberOfCentroids)
    : Input(input)
    , ClipArray(clipArray)
    , IsoValue(isoValue)
    , InsideOut(insideOut)
    , PointsMap(pointsMap)
    , CellsMap(cellsMap)
    , BatchInfo(batchInfo)
    , CellDataArrays(cellDataArrays)
    , EdgeLocator(edgeLocator)
    , ConnectivitySize(connectivitySize)
    , NumberOfOutputCells(numberOfOutputCells)
    , NumberOfKeptPoints(numberOfKeptPoints)
    , NumberOfEdges(numberOfEdges)
    , NumberOfCentroids(numberOfCentroids)
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
    // set NumberOfKeptPointsAndEdges
    this->NumberOfKeptPointsAndEdges = this->NumberOfKeptPoints + this->NumberOfEdges;

    int gridDims[3];
    this->Input->GetDimensions(gridDims);
    this->IsTwoDim = int(gridDims[0] <= 1 || gridDims[1] <= 1 || gridDims[2] <= 1);
    if (gridDims[0] <= 1)
    {
      this->TwoDimType = YZ;
    }
    else if (gridDims[1] <= 1)
    {
      this->TwoDimType = XZ;
    }
    else
    {
      this->TwoDimType = XY;
    }

    const std::array<int, 8> shiftLUTx = { 0, 1, 1, 0, 0, 1, 1, 0 };
    const std::array<int, 8> shiftLUTy = { 0, 0, 1, 1, 0, 0, 1, 1 };
    const std::array<int, 8> shiftLUTz = { 0, 0, 0, 0, 1, 1, 1, 1 };

    if (this->IsTwoDim && this->TwoDimType == XZ)
    {
      this->ShiftLUT[0] = shiftLUTx;
      this->ShiftLUT[1] = shiftLUTz;
      this->ShiftLUT[2] = shiftLUTy;
    }
    else if (this->IsTwoDim && this->TwoDimType == YZ)
    {
      this->ShiftLUT[0] = shiftLUTy;
      this->ShiftLUT[1] = shiftLUTz;
      this->ShiftLUT[2] = shiftLUTx;
    }
    else
    {
      this->ShiftLUT[0] = shiftLUTx;
      this->ShiftLUT[1] = shiftLUTy;
      this->ShiftLUT[2] = shiftLUTz;
    }

    this->CellDims = { gridDims[0] - 1, gridDims[1] - 1, gridDims[2] - 1 };
    this->CyStride = (this->CellDims[0] ? this->CellDims[0] : 1);
    this->CzStride =
      (this->CellDims[0] ? this->CellDims[0] : 1) * (this->CellDims[1] ? this->CellDims[1] : 1);
    this->PyStride = gridDims[0];
    this->PzStride = gridDims[0] * gridDims[1];
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    const auto& clipArray = vtk::DataArrayValueRange<1>(this->ClipArray);
    const auto& pointsMap = vtk::DataArrayValueRange<1>(this->PointsMap);
    const auto& cellsMap = vtk::DataArrayValueRange<1>(this->CellsMap);
    auto connectivity = vtk::DataArrayValueRange<1>(this->Connectivity);
    auto offsets = vtk::DataArrayValueRange<1>(this->Offsets);
    auto types = vtk::DataArrayValueRange<1>(this->OutputCellTypes);
    vtkIdType cellPointIndex, j, outputCellId, offset, outputCentroidId, cellId;
    vtkIdType centroidIds[4], shapeIds[MAX_CELL_SIZE];
    TInputIdType pointIndex1, pointIndex2;
    double grdDiffs[8];
    int caseIndex, theCellI, theCellJ, theCellK;
    int16_t color, centroidIndex;
    const int8_t numberOfPoints = this->IsTwoDim ? 4 : 8;
    uint16_t startIndex;
    uint8_t numberOfOutputs, shape, numberOfCellPoints, p, pointIndex, point1Index, point2Index;
    uint8_t* thisCase = nullptr;
    const EDGEIDXS* edgeVertices = nullptr;

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      const TableBasedBatch& batch = this->BatchInfo.Batches[batchId];
      outputCellId = batch.BeginCellsOffsets;
      offset = batch.BeginCellsConnectivity;
      outputCentroidId = batch.BeginCentroid;

      for (cellId = batch.BeginCellId; cellId < batch.EndCellId; ++cellId)
      {
        // process cells that has output cells (either itself or at least because it's clipped)
        if (cellsMap[cellId] == 1)
        {
          theCellI = (this->CellDims[0] > 0 ? cellId % this->CellDims[0] : 0);
          theCellJ = (this->CellDims[1] > 0 ? (cellId / this->CyStride) % this->CellDims[1] : 0);
          theCellK = (this->CellDims[2] > 0 ? (cellId / this->CzStride) : 0);

          caseIndex = 0;
          for (j = numberOfPoints - 1; j >= 0; --j)
          {
            cellPointIndex = (theCellI + this->ShiftLUT[0][j]) +
              (theCellJ + this->ShiftLUT[1][j]) * this->PyStride +
              (theCellK + this->ShiftLUT[2][j]) * this->PzStride;

            grdDiffs[j] = clipArray[cellPointIndex] - this->IsoValue;
            caseIndex += ((grdDiffs[j] >= 0.0) ? 1 : 0);
            caseIndex <<= (1 - (!j));
          }

          // start index, split case, number of output, and vertices from edges
          if (this->IsTwoDim)
          {
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesQua[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
          }
          else
          {
            startIndex = vtkTableBasedClipperClipTables::StartClipShapesHex[caseIndex];
            thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex[startIndex];
            numberOfOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[caseIndex];
            edgeVertices = vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
          }
          for (j = 0; j < numberOfOutputs; j++)
          {
            numberOfCellPoints = 0;
            color = -1;
            centroidIndex = -1;
            shape = *thisCase++;
            // number of points and color
            switch (shape)
            {
              case ST_HEX:
                numberOfCellPoints = 8;
                color = *thisCase++;
                break;

              case ST_WDG:
                numberOfCellPoints = 6;
                color = *thisCase++;
                break;

              case ST_PYR:
                numberOfCellPoints = 5;
                color = *thisCase++;
                break;

              case ST_TET:
                numberOfCellPoints = 4;
                color = *thisCase++;
                break;

              case ST_QUA:
                numberOfCellPoints = 4;
                color = *thisCase++;
                break;

              case ST_TRI:
                numberOfCellPoints = 3;
                color = *thisCase++;
                break;

              case ST_LIN:
                numberOfCellPoints = 2;
                color = *thisCase++;
                break;

              case ST_VTX:
                numberOfCellPoints = 1;
                color = *thisCase++;
                break;

              case ST_PNT:
                centroidIndex = *thisCase++;
                color = *thisCase++;
                numberOfCellPoints = *thisCase++;
                break;

              default:
                vtkLogF(ERROR, "An invalid output shape was found in the ClipCases.");
            }

            if ((!this->InsideOut && color == COLOR0) || (this->InsideOut && color == COLOR1))
            {
              // We don't want this one; it's the wrong side.
              thisCase += numberOfCellPoints;
              continue;
            }
            for (p = 0; p < numberOfCellPoints; ++p)
            {
              pointIndex = *thisCase++;

              if (pointIndex <= P7) // Input Point
              {
                // We know pt P0 must be > P0 since we already
                // assume P0 == 0.  This is why we do not
                // bother subtracting P0 from pt here.
                shapeIds[p] = pointsMap[((theCellI + this->ShiftLUT[0][pointIndex]) +
                  (theCellJ + this->ShiftLUT[1][pointIndex]) * this->PyStride +
                  (theCellK + this->ShiftLUT[2][pointIndex]) * this->PzStride)];
              }
              else if (pointIndex >= EA && pointIndex <= EL) // Mid-Edge Point
              {
                point1Index = edgeVertices[pointIndex - EA][0];
                point2Index = edgeVertices[pointIndex - EA][1];
                if (point2Index < point1Index)
                {
                  std::swap(point1Index, point2Index);
                }

                pointIndex1 = ((theCellI + this->ShiftLUT[0][point1Index]) +
                  (theCellJ + this->ShiftLUT[1][point1Index]) * this->PyStride +
                  (theCellK + this->ShiftLUT[2][point1Index]) * this->PzStride);
                pointIndex2 = ((theCellI + this->ShiftLUT[0][point2Index]) +
                  (theCellJ + this->ShiftLUT[1][point2Index]) * this->PyStride +
                  (theCellK + this->ShiftLUT[2][point2Index]) * this->PzStride);

                shapeIds[p] = this->NumberOfKeptPoints +
                  this->EdgeLocator.IsInsertedEdge(pointIndex1, pointIndex2);
              }
              else if (pointIndex >= N0 && pointIndex <= N3) // Centroid Point
              {
                shapeIds[p] = centroidIds[pointIndex - N0];
              }
              else
              {
                vtkLogF(ERROR, "An invalid output shape was found in the ClipCases.");
              }
            }

            switch (shape)
            {
              case ST_HEX:
                types[outputCellId] = VTK_HEXAHEDRON;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 8; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_WDG:
                types[outputCellId] = VTK_WEDGE;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 6; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_PYR:
                types[outputCellId] = VTK_PYRAMID;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 5; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_TET:
                types[outputCellId] = VTK_TETRA;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 4; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_QUA:
                types[outputCellId] = VTK_QUAD;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 4; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_TRI:
                types[outputCellId] = VTK_TRIANGLE;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 3; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_LIN:
                types[outputCellId] = VTK_LINE;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                for (uint8_t i = 0; i < 2; ++i)
                {
                  connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[i]);
                }
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_VTX:
                types[outputCellId] = VTK_VERTEX;
                offsets[outputCellId] = static_cast<TOutputIdType>(offset);
                connectivity[offset++] = static_cast<TOutputIdType>(shapeIds[0]);
                this->CellDataArrays.Copy(cellId, outputCellId++);
                break;

              case ST_PNT:
                this->Centroids[outputCentroidId] = Centroid(shapeIds, numberOfCellPoints);
                centroidIds[centroidIndex] = this->NumberOfKeptPointsAndEdges + outputCentroidId++;
            }
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

  void Execute()
  {
    vtkSMPTools::For(0, static_cast<vtkIdType>(this->BatchInfo.Batches.size()), *this);
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
    vtkAOSDataArrayTemplate<TInputIdType>* pointsMap, ArrayList& pointDataArrays,
    const std::vector<TEdge>& edges, const std::vector<Centroid>& centroids,
    vtkIdType numberOfKeptPoints, vtkIdType numberOfEdges, vtkIdType numberOfCentroids)
  {
    // copy kept input points
    auto extractKeptPoints = [&](vtkIdType beginPointId, vtkIdType endPointId) {
      const auto inPts = vtk::DataArrayTupleRange<3>(inputPoints);
      auto outPts = vtk::DataArrayTupleRange<3>(outputPoints);
      const auto ptsMap = vtk::DataArrayValueRange<1>(pointsMap);

      TInputIdType keptPointId;
      for (vtkIdType pointId = beginPointId; pointId < endPointId; ++pointId)
      {
        if (ptsMap[pointId] >= 0)
        {
          keptPointId = ptsMap[pointId];
          const auto inputPoint = inPts[pointId];
          auto outputPoint = outPts[keptPointId];
          outputPoint[0] = inputPoint[0];
          outputPoint[1] = inputPoint[1];
          outputPoint[2] = inputPoint[2];
          pointDataArrays.Copy(pointId, keptPointId);
        }
      }
    };
    vtkSMPTools::For(0, pointsMap->GetNumberOfValues(), extractKeptPoints);

    // create edge points
    auto extractEdgePoints = [&](vtkIdType beginEdgeId, vtkIdType endEdgeId) {
      const auto inPts = vtk::DataArrayTupleRange<3>(inputPoints);
      auto outPts = vtk::DataArrayTupleRange<3>(outputPoints);
      vtkIdType outputMidEdgePointId;

      for (vtkIdType edgeId = beginEdgeId; edgeId < endEdgeId; ++edgeId)
      {
        const TEdge& edge = edges[edgeId];
        const auto edgePoint1 = inPts[edge.V0];
        const auto edgePoint2 = inPts[edge.V1];
        outputMidEdgePointId = numberOfKeptPoints + edgeId;
        auto outputPoint = outPts[outputMidEdgePointId];

        const double& percentage = edge.Data;
        double bPercentage = 1.0 - percentage;
        outputPoint[0] = edgePoint1[0] * percentage + edgePoint2[0] * bPercentage;
        outputPoint[1] = edgePoint1[1] * percentage + edgePoint2[1] * bPercentage;
        outputPoint[2] = edgePoint1[2] * percentage + edgePoint2[2] * bPercentage;
        pointDataArrays.InterpolateEdge(edge.V0, edge.V1, bPercentage, outputMidEdgePointId);
      }
    };
    vtkSMPTools::For(0, numberOfEdges, extractEdgePoints);

    // create centroid points
    auto extractCentroids = [&](vtkIdType beginCentroid, vtkIdType endCentroid) {
      auto outPts = vtk::DataArrayTupleRange<3>(outputPoints);
      vtkIdType outputCentroidPointId;
      double weights[MAX_CELL_SIZE];
      double weightFactor;
      uint8_t i;

      for (vtkIdType centroidId = beginCentroid; centroidId < endCentroid; ++centroidId)
      {
        const Centroid& centroid = centroids[centroidId];
        outputCentroidPointId = numberOfKeptPoints + numberOfEdges + centroidId;
        auto outputPoint = outPts[outputCentroidPointId];

        outputPoint[0] = 0;
        outputPoint[1] = 0;
        outputPoint[2] = 0;
        weightFactor = 1.0 / centroid.NumberOfPoints;
        for (i = 0; i < centroid.NumberOfPoints; ++i)
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

template <typename TGrid, typename TInputIdType>
vtkSmartPointer<vtkUnstructuredGrid> ClipUnstructuredData(TGrid* input, vtkPoints* inputPoints,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue, bool insideOut,
  bool generateClipScalars, int outputPointsPrecision, unsigned int batchSize)
{
  // Evaluate points and calculate numberOfKeptPoints, pointsMap, clipArray
  EvaluatePointsWorker<TInputIdType> evaluatePointsWorker;
  if (implicitFunction)
  {
    using EvaluatePointsWithImplicitFunctionDispatch =
      vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    if (!EvaluatePointsWithImplicitFunctionDispatch::Execute(
          inputPoints->GetData(), evaluatePointsWorker, implicitFunction, isoValue, insideOut))
    {
      evaluatePointsWorker(inputPoints->GetData(), implicitFunction, isoValue, insideOut);
    }
  }
  else
  {
    evaluatePointsWorker(scalars, isoValue, insideOut);
  }
  const TInputIdType numberOfKeptPoints = evaluatePointsWorker.NumberOfKeptPoints;
  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> pointsMap = evaluatePointsWorker.PointsMap;
  vtkSmartPointer<vtkDoubleArray> clipArray = evaluatePointsWorker.ClipArray;
  if (implicitFunction && generateClipScalars)
  {
    input->GetPointData()->SetScalars(clipArray);
  }
  // check if there are no kept points
  if (numberOfKeptPoints == 0)
  {
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  // Evaluate cells and calculate connectivitySize, numberOfOutputCells, numberOfCentroids,
  // batchInfo, cellsMap, edges
  using TEdge = EdgeType<TInputIdType>;
  EvaluateCellsUnstructured<TGrid, TInputIdType> evaluateCellsUnstructured(
    input, clipArray.Get(), isoValue, insideOut, batchSize);
  evaluateCellsUnstructured.Execute();
  const vtkIdType connectivitySize = evaluateCellsUnstructured.ConnectivitySize;
  const vtkIdType numberOfOutputCells = evaluateCellsUnstructured.NumberOfOutputCells;
  const vtkIdType numberOfCentroids = evaluateCellsUnstructured.NumberOfCentroids;
  const TableBasedBatchInfo& batchInfo = evaluateCellsUnstructured.BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> cellsMap = evaluateCellsUnstructured.CellsMap;
  std::vector<TEdge> edges = std::move(evaluateCellsUnstructured.Edges);

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
  if (outputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outputPoints->SetDataType(inputPoints->GetDataType());
  }
  else if (outputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outputPoints->SetDataType(VTK_FLOAT);
  }
  else if (outputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
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
    ExtractCellsUnstructured<TGrid, TInputIdType, TOutputIdType> extractCellsUnstructured(input,
      clipArray, isoValue, insideOut, pointsMap.Get(), cellsMap.Get(), batchInfo, cellDataArrays,
      edgeLocator, connectivitySize, numberOfOutputCells, numberOfKeptPoints, numberOfEdges,
      numberOfCentroids);
    extractCellsUnstructured.Execute();
    centroids = std::move(extractCellsUnstructured.Centroids);
    outputCellTypes = extractCellsUnstructured.OutputCellTypes;
    outputCellArray = extractCellsUnstructured.OutputCellArray;
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    // Extract cells and calculate centroids, types, cell array, cell data
    ExtractCellsUnstructured<TGrid, TInputIdType, TOutputIdType> extractCellsUnstructured(input,
      clipArray, isoValue, insideOut, pointsMap.Get(), cellsMap.Get(), batchInfo, cellDataArrays,
      edgeLocator, connectivitySize, numberOfOutputCells, numberOfKeptPoints, numberOfEdges,
      numberOfCentroids);
    extractCellsUnstructured.Execute();
    centroids = std::move(extractCellsUnstructured.Centroids);
    outputCellTypes = extractCellsUnstructured.OutputCellTypes;
    outputCellArray = extractCellsUnstructured.OutputCellArray;
  }
  // Extract points and calculate outputPoints and outputPointData.
  ExtractPointsWorker<TInputIdType> extractPointsWorker;
  using ExtractPointsDispatch =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!ExtractPointsDispatch::Execute(inputPoints->GetData(), outputPoints->GetData(),
        extractPointsWorker, pointsMap.Get(), pointDataArrays, edges, centroids, numberOfKeptPoints,
        numberOfEdges, numberOfCentroids))
  {
    extractPointsWorker(inputPoints->GetData(), outputPoints->GetData(), pointsMap.Get(),
      pointDataArrays, edges, centroids, numberOfKeptPoints, numberOfEdges, numberOfCentroids);
  }

  // create outputClippedCells
  auto outputClippedCells = vtkSmartPointer<vtkUnstructuredGrid>::New();
  outputClippedCells->SetPoints(outputPoints);
  outputClippedCells->GetPointData()->ShallowCopy(outputPointData);
  outputClippedCells->SetCells(outputCellTypes, outputCellArray, nullptr, nullptr);
  outputClippedCells->GetCellData()->ShallowCopy(outputCellData);

  return outputClippedCells;
}

template <typename TGrid, typename TInputIdType>
vtkSmartPointer<vtkUnstructuredGrid> ClipStructuredData(TGrid* input, vtkPoints* inputPoints,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue, bool insideOut,
  bool generateClipScalars, int outputPointsPrecision, unsigned int batchSize)
{
  // Evaluate points and calculate numberOfKeptPoints, pointsMap, clipArray
  EvaluatePointsWorker<TInputIdType> evaluatePointsWorker;
  if (implicitFunction)
  {
    using EvaluatePointsWithImplicitFunctionDispatch =
      vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    if (!EvaluatePointsWithImplicitFunctionDispatch::Execute(
          inputPoints->GetData(), evaluatePointsWorker, implicitFunction, isoValue, insideOut))
    {
      evaluatePointsWorker(inputPoints->GetData(), implicitFunction, isoValue, insideOut);
    }
  }
  else
  {
    evaluatePointsWorker(scalars, isoValue, insideOut);
  }
  const TInputIdType numberOfKeptPoints = evaluatePointsWorker.NumberOfKeptPoints;
  vtkSmartPointer<vtkAOSDataArrayTemplate<TInputIdType>> pointsMap = evaluatePointsWorker.PointsMap;
  vtkSmartPointer<vtkDoubleArray> clipArray = evaluatePointsWorker.ClipArray;
  if (implicitFunction && generateClipScalars)
  {
    input->GetPointData()->SetScalars(clipArray);
  }
  // check if there are no kept points
  if (numberOfKeptPoints == 0)
  {
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  // Evaluate cells and calculate connectivitySize, numberOfOutputCells, numberOfCentroids,
  // batchInfo, cellsMap, edges
  using TEdge = EdgeType<TInputIdType>;
  EvaluateCellsStructured<TGrid, TInputIdType> evaluateCellsStructured(
    input, clipArray.Get(), isoValue, insideOut, batchSize);
  evaluateCellsStructured.Execute();
  const vtkIdType connectivitySize = evaluateCellsStructured.ConnectivitySize;
  const vtkIdType numberOfOutputCells = evaluateCellsStructured.NumberOfOutputCells;
  const vtkIdType numberOfCentroids = evaluateCellsStructured.NumberOfCentroids;
  const TableBasedBatchInfo& batchInfo = evaluateCellsStructured.BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> cellsMap = evaluateCellsStructured.CellsMap;
  std::vector<TEdge> edges = std::move(evaluateCellsStructured.Edges);

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
  if (outputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outputPoints->SetDataType(inputPoints->GetDataType());
  }
  else if (outputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    outputPoints->SetDataType(VTK_FLOAT);
  }
  else if (outputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
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
  outputCellData->InterpolateAllocate(input->GetCellData(), numberOfOutputCells);
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
    ExtractCellsStructured<TGrid, TInputIdType, TOutputIdType> extractCellsStructured(input,
      clipArray, isoValue, insideOut, pointsMap.Get(), cellsMap.Get(), batchInfo, cellDataArrays,
      edgeLocator, connectivitySize, numberOfOutputCells, numberOfKeptPoints, numberOfEdges,
      numberOfCentroids);
    extractCellsStructured.Execute();
    centroids = std::move(extractCellsStructured.Centroids);
    outputCellTypes = extractCellsStructured.OutputCellTypes;
    outputCellArray = extractCellsStructured.OutputCellArray;
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    // Extract cells and calculate centroids, types, cell array, cell data
    ExtractCellsStructured<TGrid, TInputIdType, TOutputIdType> extractCellsStructured(input,
      clipArray, isoValue, insideOut, pointsMap.Get(), cellsMap.Get(), batchInfo, cellDataArrays,
      edgeLocator, connectivitySize, numberOfOutputCells, numberOfKeptPoints, numberOfEdges,
      numberOfCentroids);
    extractCellsStructured.Execute();
    centroids = std::move(extractCellsStructured.Centroids);
    outputCellTypes = extractCellsStructured.OutputCellTypes;
    outputCellArray = extractCellsStructured.OutputCellArray;
  }
  // Extract points and calculate outputPoints and outputPointData.
  ExtractPointsWorker<TInputIdType> extractPointsWorker;
  using ExtractPointsDispatch =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!ExtractPointsDispatch::Execute(inputPoints->GetData(), outputPoints->GetData(),
        extractPointsWorker, pointsMap.Get(), pointDataArrays, edges, centroids, numberOfKeptPoints,
        numberOfEdges, numberOfCentroids))
  {
    extractPointsWorker(inputPoints->GetData(), outputPoints->GetData(), pointsMap.Get(),
      pointDataArrays, edges, centroids, numberOfKeptPoints, numberOfEdges, numberOfCentroids);
  }

  // create outputClippedCells
  auto outputClippedCells = vtkSmartPointer<vtkUnstructuredGrid>::New();
  outputClippedCells->SetPoints(outputPoints);
  outputClippedCells->GetPointData()->ShallowCopy(outputPointData);
  outputClippedCells->SetCells(outputCellTypes, outputCellArray, nullptr, nullptr);
  outputClippedCells->GetCellData()->ShallowCopy(outputCellData);

  return outputClippedCells;
}

//------------------------------------------------------------------------------
struct FullyProcessUnstructuredDataFunctor
{
  vtkDataSet* Input;
  unsigned char CanFullyProcess;
  vtkSMPThreadLocal<unsigned char> TLCanFullyProcess;

  FullyProcessUnstructuredDataFunctor(vtkDataSet* input)
    : Input(input)
  {
    // build cells for polydata so that you can use GetCellPoints()
    vtkNew<vtkGenericCell> cell;
    this->Input->GetCell(0, cell);
  }

  void Initialize() { this->TLCanFullyProcess.Local() = 1; }

  void operator()(vtkIdType beginCellId, vtkIdType endCellId)
  {
    if (this->TLCanFullyProcess.Local() == 0)
    {
      return;
    }
    bool canBeClippedFast;
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      switch (this->Input->GetCellType(cellId))
      {
        case VTK_VOXEL:
        case VTK_HEXAHEDRON:
        case VTK_WEDGE:
        case VTK_PYRAMID:
        case VTK_TETRA:
        case VTK_PIXEL:
        case VTK_QUAD:
        case VTK_TRIANGLE:
        case VTK_LINE:
        case VTK_VERTEX:
          canBeClippedFast = true;
          break;
        default:
          canBeClippedFast = false;
          break;
      }
      if (!canBeClippedFast)
      {
        // Unsupported cell type, can't process data
        this->TLCanFullyProcess.Local() = 0;
        break;
      }
    }
  }

  void Reduce()
  {
    this->CanFullyProcess = 1;
    for (const auto& canFullyProcess : this->TLCanFullyProcess)
    {
      if (canFullyProcess == 0)
      {
        this->CanFullyProcess = 0;
        return;
      }
    }
  }
};
} // end anonymous namespace

//------------------------------------------------------------------------------
bool vtkTableBasedClipDataSet::CanFullyProcessUnstructuredData(vtkDataSet* inputGrid)
{
  if (inputGrid->GetNumberOfPoints() == 0 || inputGrid->GetNumberOfCells() == 0)
  {
    return false;
  }
  FullyProcessUnstructuredDataFunctor functor(inputGrid);
  vtkSMPTools::For(0, inputGrid->GetNumberOfCells(), functor);
  return static_cast<bool>(functor.CanFullyProcess);
}

//------------------------------------------------------------------------------
namespace
{
struct BuildCellTypesImpl
{
  // Given a polyData cell array and a size to type functor, it creates the cell types
  template <typename CellStateT, typename SizeToTypeFunctor>
  void operator()(CellStateT& state, vtkUnsignedCharArray* cellTypes, SizeToTypeFunctor&& typer)
  {
    const vtkIdType numCells = state.GetNumberOfCells();
    if (numCells == 0)
    {
      return;
    }

    vtkSMPTools::For(0, numCells, [&](vtkIdType begin, vtkIdType end) {
      auto types = cellTypes->GetPointer(0);
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        types[cellId] = static_cast<unsigned char>(typer(state.GetCellSize(cellId)));
      }
    });
  }
};
} // end anonymous namespace

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipPolyData(vtkDataSet* inputGrid,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue,
  vtkUnstructuredGrid* outputUG)
{
  // check if it's easily convertible to vtkUnstructuredGrid
  auto polyData = vtkPolyData::SafeDownCast(inputGrid);
  bool hasOnlyVerts = polyData->GetVerts()->GetNumberOfCells() != 0 &&
    polyData->GetLines()->GetNumberOfCells() == 0 &&
    polyData->GetPolys()->GetNumberOfCells() == 0 && polyData->GetStrips()->GetNumberOfCells() == 0;
  bool hasOnlyLines = polyData->GetVerts()->GetNumberOfCells() == 0 &&
    polyData->GetLines()->GetNumberOfCells() != 0 &&
    polyData->GetPolys()->GetNumberOfCells() == 0 && polyData->GetStrips()->GetNumberOfCells() == 0;
  bool hasOnlyPolys = polyData->GetVerts()->GetNumberOfCells() == 0 &&
    polyData->GetLines()->GetNumberOfCells() == 0 &&
    polyData->GetPolys()->GetNumberOfCells() != 0 && polyData->GetStrips()->GetNumberOfCells() == 0;
  bool hasOnlyStrips = polyData->GetVerts()->GetNumberOfCells() == 0 &&
    polyData->GetLines()->GetNumberOfCells() == 0 &&
    polyData->GetPolys()->GetNumberOfCells() == 0 && polyData->GetStrips()->GetNumberOfCells() != 0;
  bool easilyConvertibleToUGrid = hasOnlyVerts || hasOnlyLines || hasOnlyPolys || hasOnlyStrips;
  if (easilyConvertibleToUGrid)
  {
    // convert to vtkUnstructuredGrid
    //
    // It's beneficial to convert a polydata to unstructured grid for clipping because the
    // GetCellType and GetCellPoints are the most expensive functions used (excluding point/cell
    // data related functions). The vtkPolyData ones are more expensive than the vtkUnstructuredGrid
    // ones because they perform a bit operation to get the cell type and then based on that, get
    // the correct cell array and extract the cell points. This overhead turns out to increase the
    // execution time by 10%-20%.
    vtkNew<vtkUnstructuredGrid> uGrid;
    vtkNew<vtkUnsignedCharArray> cellTypes;
    cellTypes->SetNumberOfValues(inputGrid->GetNumberOfCells());
    uGrid->SetPoints(polyData->GetPoints());
    uGrid->GetPointData()->ShallowCopy(polyData->GetPointData());
    if (hasOnlyVerts)
    {
      polyData->GetVerts()->Visit(BuildCellTypesImpl{}, cellTypes,
        [](vtkIdType size) -> VTKCellType { return size == 1 ? VTK_VERTEX : VTK_POLY_VERTEX; });
      uGrid->SetCells(cellTypes, polyData->GetVerts(), nullptr, nullptr);
    }
    else if (hasOnlyLines)
    {
      polyData->GetLines()->Visit(BuildCellTypesImpl{}, cellTypes,
        [](vtkIdType size) -> VTKCellType { return size == 2 ? VTK_LINE : VTK_POLY_LINE; });
      uGrid->SetCells(cellTypes, polyData->GetLines(), nullptr, nullptr);
    }
    else if (hasOnlyPolys)
    {
      polyData->GetPolys()->Visit(
        BuildCellTypesImpl{}, cellTypes, [](vtkIdType size) -> VTKCellType {
          switch (size)
          {
            case 3:
              return VTK_TRIANGLE;
            case 4:
              return VTK_QUAD;
            default:
              return VTK_POLYGON;
          }
        });
      uGrid->SetCells(cellTypes, polyData->GetPolys(), nullptr, nullptr);
    }
    else // hasOnlyStrips
    {
      polyData->GetStrips()->Visit(BuildCellTypesImpl{}, cellTypes,
        [](vtkIdType vtkNotUsed(size)) -> VTKCellType { return VTK_TRIANGLE_STRIP; });
      uGrid->SetCells(cellTypes, polyData->GetStrips(), nullptr, nullptr);
    }
    uGrid->GetCellData()->ShallowCopy(polyData->GetCellData());
    this->ClipUnstructuredGrid(uGrid, implicitFunction, scalars, isoValue, outputUG);
  }
  else
  {
    if (!this->CanFullyProcessUnstructuredData(inputGrid))
    {
      this->ClipDataSet(inputGrid, outputUG);
      return;
    }
    vtkPoints* inputPoints = polyData->GetPoints();
    vtkSmartPointer<vtkUnstructuredGrid> clippedOutput;
#ifdef VTK_USE_64BIT_IDS
    const vtkIdType numberOfPoints = inputPoints->GetNumberOfPoints();
    bool use64BitsIds = (numberOfPoints > VTK_TYPE_INT32_MAX);
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      clippedOutput = ClipUnstructuredData<vtkPolyData, TInputIdType>(polyData, inputPoints,
        implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
        this->OutputPointsPrecision, this->BatchSize);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      clippedOutput = ClipUnstructuredData<vtkPolyData, TInputIdType>(polyData, inputPoints,
        implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
        this->OutputPointsPrecision, this->BatchSize);
    }
    outputUG->ShallowCopy(clippedOutput);
  }
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipUnstructuredGrid(vtkDataSet* inputGrid,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue,
  vtkUnstructuredGrid* outputUG)
{
  if (!this->CanFullyProcessUnstructuredData(inputGrid))
  {
    this->ClipDataSet(inputGrid, outputUG);
    return;
  }
  auto uGrid = vtkUnstructuredGridBase::SafeDownCast(inputGrid);
  vtkPoints* inputPoints = uGrid->GetPoints();
  vtkSmartPointer<vtkUnstructuredGrid> clippedOutput;
#ifdef VTK_USE_64BIT_IDS
  const vtkIdType numberOfPoints = inputPoints->GetNumberOfPoints();
  bool use64BitsIds = (numberOfPoints > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkTypeInt64;
    clippedOutput = ClipUnstructuredData<vtkUnstructuredGridBase, TInputIdType>(uGrid, inputPoints,
      implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
      this->OutputPointsPrecision, this->BatchSize);
  }
  else
#endif
  {
    using TInputIdType = vtkTypeInt32;
    clippedOutput = ClipUnstructuredData<vtkUnstructuredGridBase, TInputIdType>(uGrid, inputPoints,
      implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
      this->OutputPointsPrecision, this->BatchSize);
  }
  outputUG->ShallowCopy(clippedOutput);
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipImageData(vtkDataSet* inputGrid,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue,
  vtkUnstructuredGrid* outputUG)
{
  auto imageData = vtkImageData::SafeDownCast(inputGrid);
  int i, j;
  int dataDims[3];
  double spacings[3];
  double tmpValue;
  imageData->GetDimensions(dataDims);
  imageData->GetSpacing(spacings);
  const double* dataBBox = imageData->GetBounds();
  vtkNew<vtkDoubleArray> pxCoords;
  vtkNew<vtkDoubleArray> pyCoords;
  vtkNew<vtkDoubleArray> pzCoords;
  vtkDoubleArray* tmpArrays[3] = { pxCoords.Get(), pyCoords.Get(), pzCoords.Get() };
  for (j = 0; j < 3; j++)
  {
    tmpArrays[j]->SetNumberOfComponents(1);
    tmpArrays[j]->SetNumberOfTuples(dataDims[j]);
    for (tmpValue = dataBBox[j << 1], i = 0; i < dataDims[j]; i++, tmpValue += spacings[j])
    {
      tmpArrays[j]->SetValue(i, tmpValue);
    }
    tmpArrays[j] = nullptr;
  }

  vtkNew<vtkRectilinearGrid> rectGrid;
  rectGrid->SetDimensions(dataDims);
  rectGrid->SetXCoordinates(pxCoords);
  rectGrid->SetYCoordinates(pyCoords);
  rectGrid->SetZCoordinates(pzCoords);
  rectGrid->GetPointData()->ShallowCopy(imageData->GetPointData());
  rectGrid->GetCellData()->ShallowCopy(imageData->GetCellData());
  this->ClipRectilinearGrid(rectGrid, implicitFunction, scalars, isoValue, outputUG);
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipRectilinearGrid(vtkDataSet* inputGrid,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue,
  vtkUnstructuredGrid* outputUG)
{
  auto rectilinearGrid = vtkRectilinearGrid::SafeDownCast(inputGrid);

  vtkNew<vtkPoints> inputPoints;
  rectilinearGrid->GetPoints(inputPoints);

  vtkSmartPointer<vtkUnstructuredGrid> clippedOutput;
#ifdef VTK_USE_64BIT_IDS
  const vtkIdType numberOfPoints = rectilinearGrid->GetNumberOfPoints();
  bool use64BitsIds = (numberOfPoints > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkTypeInt64;
    clippedOutput = ClipStructuredData<vtkRectilinearGrid, TInputIdType>(rectilinearGrid,
      inputPoints, implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
      this->OutputPointsPrecision, this->BatchSize);
  }
  else
#endif
  {
    using TInputIdType = vtkTypeInt32;
    clippedOutput = ClipStructuredData<vtkRectilinearGrid, TInputIdType>(rectilinearGrid,
      inputPoints, implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
      this->OutputPointsPrecision, this->BatchSize);
  }
  outputUG->ShallowCopy(clippedOutput);
}

//------------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipStructuredGrid(vtkDataSet* inputGrid,
  vtkImplicitFunction* implicitFunction, vtkDoubleArray* scalars, double isoValue,
  vtkUnstructuredGrid* outputUG)
{
  auto structuredGrid = vtkStructuredGrid::SafeDownCast(inputGrid);

  vtkPoints* inputPoints = structuredGrid->GetPoints();
  vtkSmartPointer<vtkUnstructuredGrid> clippedOutput;
#ifdef VTK_USE_64BIT_IDS
  const vtkIdType numberOfPoints = inputPoints->GetNumberOfPoints();
  bool use64BitsIds = (numberOfPoints > VTK_TYPE_INT32_MAX);
  if (use64BitsIds)
  {
    using TInputIdType = vtkTypeInt64;
    clippedOutput = ClipStructuredData<vtkStructuredGrid, TInputIdType>(structuredGrid, inputPoints,
      implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
      this->OutputPointsPrecision, this->BatchSize);
  }
  else
#endif
  {
    using TInputIdType = vtkTypeInt32;
    clippedOutput = ClipStructuredData<vtkStructuredGrid, TInputIdType>(structuredGrid, inputPoints,
      implicitFunction, scalars, isoValue, this->InsideOut, this->GenerateClipScalars,
      this->OutputPointsPrecision, this->BatchSize);
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
