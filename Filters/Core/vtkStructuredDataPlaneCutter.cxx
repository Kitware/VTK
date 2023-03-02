/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredDataPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredDataPlaneCutter.h"

#include "vtkAbstractTransform.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkFlyingEdgesPlaneCutter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMarchingCubesPolygonCases.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPTools.h"
#include "vtkSphereTree.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkStructuredDataPlaneCutter);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkStructuredDataPlaneCutter, Plane, vtkPlane);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkStructuredDataPlaneCutter, SphereTree, vtkSphereTree);

//------------------------------------------------------------------------------
// Construct an instance of the class.
vtkStructuredDataPlaneCutter::vtkStructuredDataPlaneCutter()
{
  this->Plane = vtkPlane::New();
  this->InputInfo = vtkInputInfo(nullptr, 0);
}

//------------------------------------------------------------------------------
vtkStructuredDataPlaneCutter::~vtkStructuredDataPlaneCutter()
{
  this->SetPlane(nullptr);
  this->SetSphereTree(nullptr);
  this->InputInfo = vtkInputInfo(nullptr, 0);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtkStructuredDataPlaneCutter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Plane != nullptr)
  {
    vtkMTimeType mTime2 = this->Plane->GetMTime();
    mTime = std::max(mTime2, mTime);
  }
  if (this->SphereTree != nullptr)
  {
    vtkMTimeType mTime2 = this->SphereTree->GetMTime();
    mTime = std::max(mTime2, mTime);
  }
  return mTime;
}

//------------------------------------------------------------------------------
int vtkStructuredDataPlaneCutter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkStructuredDataPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";
  if (this->SphereTree)
  {
    os << indent << "SphereTree: " << this->SphereTree << "\n";
  }
  else
  {
    os << indent << "SphereTree: (none)\n";
  }
  os << indent << "Interpolate Attributes: " << (this->InterpolateAttributes ? "On\n" : "Off\n");
  os << indent << "Generate Polygons: " << (this->GeneratePolygons ? "On\n" : "Off\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Build Tree: " << (this->BuildTree ? "On\n" : "Off\n");
  os << indent << "Build Hierarchy: " << (this->BuildHierarchy ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
  os << indent << "Batch size: " << this->BatchSize << "\n";
}

//------------------------------------------------------------------------------
int vtkStructuredDataPlaneCutter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}
namespace
{
//------------------------------------------------------------------------------
// Extract the sliced cells is a 4-step process
// 1) Determine which input points will be kept and calculate Evaluate points
//    and sliceArray.
//    1) Step 1 can be skipped if there is sphere tree which is built instead
// 2) Evaluate the input cells and calculate connectivitySize, numberOfOutputCells
//    batchInfo, cellsMap, edges
// 3) Extract cells and calculate cell array, cell data
// 4) Extract points and point data

//-----------------------------------------------------------------------------
// Evaluate the plane equation for each input point.
template <typename TPointsArray>
struct EvaluatePointsWithPlaneFunctor
{
  TPointsArray* PointsArray;
  double* Origin;
  double* Normal;

  vtkSmartPointer<vtkUnsignedCharArray> InOutArray;
  vtkSmartPointer<vtkDoubleArray> SliceArray;

  EvaluatePointsWithPlaneFunctor(TPointsArray* pointsArray, double* origin, double* normal)
    : PointsArray(pointsArray)
    , Origin(origin)
    , Normal(normal)
  {
    this->InOutArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->InOutArray->SetNumberOfValues(this->PointsArray->GetNumberOfTuples());
    this->SliceArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->SliceArray->SetNumberOfValues(this->PointsArray->GetNumberOfTuples());
  }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId)
  {
    double p[3], zero = double(0);
    const auto& points = vtk::DataArrayTupleRange<3>(this->PointsArray, beginPtId, endPtId);
    auto inOut = vtk::DataArrayValueRange<1>(this->InOutArray, beginPtId, endPtId);
    auto slice = vtk::DataArrayValueRange<1>(this->SliceArray, beginPtId, endPtId);
    auto pointsItr = points.begin();
    auto inOutItr = inOut.begin();
    auto sliceItr = slice.begin();
    for (; pointsItr != points.end(); ++pointsItr, ++inOutItr, ++sliceItr)
    {
      // Access each point
      p[0] = static_cast<double>((*pointsItr)[0]);
      p[1] = static_cast<double>((*pointsItr)[1]);
      p[2] = static_cast<double>((*pointsItr)[2]);

      // Evaluate position of the point with the plane. Invoke inline,
      // non-virtual version of evaluate method.
      *sliceItr = vtkPlane::Evaluate(this->Normal, this->Origin, p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *inOutItr = (*sliceItr > zero ? 2 : (*sliceItr < zero ? 1 : 0));
    }
  }
};

//-----------------------------------------------------------------------------
struct EvaluatePointsWithPlaneWorker
{
  vtkSmartPointer<vtkUnsignedCharArray> InOutArray;
  vtkSmartPointer<vtkDoubleArray> SliceArray;

  template <typename TPointsArray>
  void operator()(TPointsArray* pointsArray, double* origin, double* normal)
  {
    EvaluatePointsWithPlaneFunctor<TPointsArray> evaluatePoints(pointsArray, origin, normal);
    vtkSMPTools::For(0, pointsArray->GetNumberOfTuples(), evaluatePoints);
    this->InOutArray = evaluatePoints.InOutArray;
    this->SliceArray = evaluatePoints.SliceArray;
  }
};

//-----------------------------------------------------------------------------
// Keep track of output information within each batch of cells - this
// information is eventually rolled up into offsets into the cell
// connectivity and offsets arrays so that separate threads know where to
// write their data. We need to know how many total cells are created, the
// number of lines generated (which is equal to the number of slices cells),
// and the connectivity size of the output cells and lines.
struct SliceBatch
{
  // These are accumulated in EvaluateCells::operator().
  vtkIdType NumberOfCells;
  vtkIdType CellsConnectivitySize;
  // These are needed because SliceBatchInfo will preserve only the batches
  // with NumberOfCells > 0
  vtkIdType BeginCellId;
  vtkIdType EndCellId;

  // These are assigned via prefix sum in EvaluateCells::Reduce(). This
  // information is used to instantiate the output cell arrays,
  vtkIdType BeginCellsOffsets;
  vtkIdType BeginCellsConnectivity;

  SliceBatch()
    : NumberOfCells(0)
    , CellsConnectivitySize(0)
    , BeginCellId(0)
    , EndCellId(0)
    , BeginCellsOffsets(0)
    , BeginCellsConnectivity(0)
  {
  }
};

//-----------------------------------------------------------------------------
struct SliceBatchInfo
{
  unsigned int BatchSize;
  std::vector<SliceBatch> Batches;
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
const int CASE_MASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

//------------------------------------------------------------------------------
const int EDGE_CASE[12][2] = { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, { 4, 5 }, { 5, 6 }, { 7, 6 },
  { 4, 7 }, { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 } };

//------------------------------------------------------------------------------
void ComputeCellPointIds(const vtkIdType& cellId, vtkIdType cellIds[8], const int cellDimensions[3],
  const int dimensions[3], const vtkIdType& cellSliceOffset, const vtkIdType& sliceOffset)
{
  auto cellI = cellId % cellDimensions[0];
  auto cellJ = (cellId / cellDimensions[0]) % cellDimensions[1];
  auto cellK = cellId / cellSliceOffset;
  auto pointId = cellI + cellJ * dimensions[0] + cellK * sliceOffset;

  cellIds[0] = pointId;
  cellIds[1] = cellIds[0] + 1;
  cellIds[2] = cellIds[0] + 1 + dimensions[0];
  cellIds[3] = cellIds[0] + dimensions[0];
  cellIds[4] = cellIds[0] + sliceOffset;
  cellIds[5] = cellIds[1] + sliceOffset;
  cellIds[6] = cellIds[2] + sliceOffset;
  cellIds[7] = cellIds[3] + sliceOffset;
}

//------------------------------------------------------------------------------
bool SkipCell(const double s[8])
{
  return (s[0] >= 0.0 && s[1] >= 0.0 && s[2] >= 0.0 && s[3] >= 0.0 && s[4] >= 0.0 && s[5] >= 0.0 &&
           s[6] >= 0.0 && s[7] >= 0.0) ||
    (s[0] < 0.0 && s[1] < 0.0 && s[2] < 0.0 && s[3] < 0.0 && s[4] < 0.0 && s[5] < 0.0 &&
      s[6] < 0.0 && s[7] < 0.0);
}

//------------------------------------------------------------------------------
int* GetEdge(const bool& generatePolygon, const int& caseIndex)
{
  if (generatePolygon)
  {
    return &(*(vtkMarchingCubesPolygonCases::GetCases() + caseIndex)->edges);
  }
  else
  {
    return &(*(vtkMarchingCubesTriangleCases::GetCases() + caseIndex)->edges);
  }
}

//-----------------------------------------------------------------------------
// Evaluate structured cells and calculate connectivitySize, numberOfOutputCells,
// numberOfCentroids, batchInfo, cellsMap, edges
template <typename TGrid, typename TPointsArray, typename TInputIdType>
struct EvaluateCellsStructuredFunctor
{
  using TEdge = EdgeType<TInputIdType>;

  TGrid* Input;
  TPointsArray* InPointsArray;
  double* Origin;
  double* Normal;
  const unsigned char* Selected;
  const unsigned char* InOut;
  const double* Slice;
  bool GeneratePolygons;
  bool AllCellsVisible;
  const unsigned int BatchSize;
  vtkIdType NumberOfInputCells;

  int Dimensions[3];
  int CellDimensions[3];
  vtkIdType SliceOffset;
  vtkIdType CellSliceOffset;

  vtkSMPThreadLocal<std::vector<TEdge>> TLEdges;

  SliceBatchInfo BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> CellsMap;
  std::vector<TEdge> Edges;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;

  EvaluateCellsStructuredFunctor(TGrid* input, TPointsArray* pointsArray, double* origin,
    double* normal, const unsigned char* selected, const unsigned char* inOut, const double* slice,
    bool generatePolygons, bool allCellsVisible, unsigned int batchSize)
    : Input(input)
    , InPointsArray(pointsArray)
    , Origin(origin)
    , Normal(normal)
    , Selected(selected)
    , InOut(inOut)
    , Slice(slice)
    , GeneratePolygons(generatePolygons)
    , AllCellsVisible(allCellsVisible)
    , BatchSize(batchSize)
    , NumberOfInputCells(input->GetNumberOfCells())
  {
    // initialize batches
    this->BatchInfo.BatchSize = batchSize;
    size_t numberOfBatches = static_cast<size_t>(((this->NumberOfInputCells - 1) / batchSize) + 1);
    this->BatchInfo.Batches.resize(numberOfBatches);
    // initialize cellsMap
    this->CellsMap = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->CellsMap->SetNumberOfValues(this->NumberOfInputCells);

    this->Input->GetDimensions(this->Dimensions);
    this->CellDimensions[0] = this->Dimensions[0] - 1;
    this->CellDimensions[1] = this->Dimensions[1] - 1;
    this->CellDimensions[2] = this->Dimensions[2] - 1;
    this->SliceOffset = static_cast<vtkIdType>(this->Dimensions[0]) * this->Dimensions[1];
    this->CellSliceOffset =
      static_cast<vtkIdType>(this->CellDimensions[0]) * this->CellDimensions[1];
  }

  void Initialize()
  {
    // initialize edges
    this->TLEdges.Local().reserve(static_cast<size_t>(this->Input->GetNumberOfPoints() * 0.001));
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto& edges = this->TLEdges.Local();
    auto points = vtk::DataArrayTupleRange<3>(this->InPointsArray);
    auto cellsMap = vtk::DataArrayValueRange<1>(this->CellsMap);
    vtkIdType cellId, batchSize, npts, numberOfCells, cellsConnectivitySize;
    TInputIdType pointIndex1, pointIndex2;
    vtkIdType cellIds[8];
    int* edge;
    int caseIndex, point1Index, point2Index, i;
    double s[8], point1ToPoint2, point1ToIso, point1Weight;
    bool needCell;

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      SliceBatch& batch = this->BatchInfo.Batches[batchId];
      batchSize = static_cast<vtkIdType>(this->BatchInfo.BatchSize);
      batch.BeginCellId = batchId * batchSize;
      batch.EndCellId =
        (batch.BeginCellId + batchSize > this->NumberOfInputCells ? this->NumberOfInputCells
                                                                  : batch.BeginCellId + batchSize);
      const unsigned char* selected = nullptr;
      if (this->Selected)
      {
        selected = this->Selected + batch.BeginCellId;
      }
      // Traverse this batch of cells (whose bounding sphere possibly
      // intersects the plane).
      for (cellId = batch.BeginCellId; cellId < batch.EndCellId; ++cellId)
      {
        needCell = false;
        if (this->AllCellsVisible || this->Input->IsCellVisible(cellId))
        {
          if (this->Selected)
          {
            if (*selected++)
            {
              needCell = true;
            }
          }
          else // this->InOut
          {
            ::ComputeCellPointIds(cellId, cellIds, this->CellDimensions, this->Dimensions,
              this->CellSliceOffset, this->SliceOffset);

            // ArePointsAroundPlane
            unsigned char onOneSideOfPlane = this->InOut[cellIds[0]];
            for (i = 1; onOneSideOfPlane && i < 8; ++i)
            {
              onOneSideOfPlane &= this->InOut[cellIds[i]];
            }
            needCell = (!onOneSideOfPlane);
          }
        }

        numberOfCells = 0;
        cellsConnectivitySize = 0;
        if (needCell)
        {
          if (this->Selected)
          {
            ::ComputeCellPointIds(cellId, cellIds, this->CellDimensions, this->Dimensions,
              this->CellSliceOffset, this->SliceOffset);

            // Get the slice values
            for (i = 0; i < 8; ++i)
            {
              const auto& cellPoint = points[cellIds[i]];
              s[i] = (cellPoint[0] - this->Origin[0]) * this->Normal[0] +
                (cellPoint[1] - this->Origin[1]) * this->Normal[1] +
                (cellPoint[2] - this->Origin[2]) * this->Normal[2];
            }
          }
          else // this->InOut
          {
            // Get the slice values
            for (i = 0; i < 8; ++i)
            {
              s[i] = this->Slice[cellIds[i]];
            }
          }

          // Return if we are not producing anything
          if (::SkipCell(s))
          {
            cellsMap[cellId] = 0;
            continue;
          }

          // Build the case table and start producing sn output polygon as necessary
          for (i = 0, caseIndex = 0; i < 8; ++i)
          {
            if (s[i] >= 0.0)
            {
              caseIndex |= CASE_MASK[i];
            }
          }

          edge = ::GetEdge(this->GeneratePolygons, caseIndex);

          // Produce the intersections
          while (*edge > -1) // for all polygons/triangles
          {
            npts = this->GeneratePolygons ? *edge++ : 3;
            numberOfCells += (npts > 0 ? 1 : 0);
            cellsConnectivitySize += npts;
            // start polygon/triangle edge intersections
            for (i = 0; i < npts; i++, ++edge)
            {
              point1Index = EDGE_CASE[*edge][0];
              point2Index = EDGE_CASE[*edge][1];
              if (point2Index < point1Index)
              {
                std::swap(point1Index, point2Index);
              }
              point1ToPoint2 = s[point2Index] - s[point1Index];
              point1ToIso = 0.0 - s[point1Index];
              point1Weight = 1.0 - point1ToIso / point1ToPoint2;

              pointIndex1 = static_cast<TInputIdType>(cellIds[point1Index]);
              pointIndex2 = static_cast<TInputIdType>(cellIds[point2Index]);
              // swap in case the order is wrong
              if (pointIndex1 > pointIndex2)
              {
                std::swap(pointIndex1, pointIndex2);
                point1Weight = 1.0 - point1Weight;
              }
              edges.emplace_back(pointIndex1, pointIndex2, point1Weight);
            } // for all edges of polygon/triangle
          }   // for each polygon/triangle
        }
        batch.NumberOfCells += numberOfCells;
        batch.CellsConnectivitySize += cellsConnectivitySize;
        cellsMap[cellId] = numberOfCells > 0 ? 1 : 0;
      }
    }
  }

  void Reduce()
  {
    this->ConnectivitySize = 0;
    this->NumberOfOutputCells = 0;
    vtkIdType beginCellsOffsets = 0, beginCellsConnectivity = 0;

    // assign BeginCellsOffsets/BeginCellsConnectivity for each batch
    // and remove the batch with 0 cells (in-place)
    size_t batchWithOutputCellsIndex = 0;
    for (size_t i = 0; i < this->BatchInfo.Batches.size(); ++i)
    {
      auto& batch = this->BatchInfo.Batches[i];
      if (batch.NumberOfCells > 0)
      {
        batch.BeginCellsOffsets = beginCellsOffsets;
        batch.BeginCellsConnectivity = beginCellsConnectivity;

        beginCellsOffsets += batch.NumberOfCells;
        beginCellsConnectivity += batch.CellsConnectivitySize;

        this->NumberOfOutputCells += batch.NumberOfCells;
        this->ConnectivitySize += batch.CellsConnectivitySize;
        if (i != batchWithOutputCellsIndex)
        {
          this->BatchInfo.Batches[batchWithOutputCellsIndex] = batch;
        }
        batchWithOutputCellsIndex++;
      }
    }
    this->BatchInfo.Batches.resize(batchWithOutputCellsIndex);
    // store TLEdges in a vector
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
};

//-----------------------------------------------------------------------------
template <typename TGrid, typename TInputIdType>
struct EvaluateCellsStructuredWorker
{
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  SliceBatchInfo BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> CellsMap;
  using TEdge = EdgeType<TInputIdType>;
  std::vector<TEdge> Edges;

  template <typename TPointsArray>
  void operator()(TPointsArray* pointsArray, TGrid* input, double* origin, double* normal,
    const unsigned char* selected, const unsigned char* inOut, const double* slice,
    bool generatePolygons, bool allCellsVisible, unsigned int batchSize)
  {
    EvaluateCellsStructuredFunctor<TGrid, TPointsArray, TInputIdType> evaluateCellsStructured(input,
      pointsArray, origin, normal, selected, inOut, slice, generatePolygons, allCellsVisible,
      batchSize);
    vtkSMPTools::For(0, static_cast<vtkIdType>(evaluateCellsStructured.BatchInfo.Batches.size()),
      evaluateCellsStructured);
    this->ConnectivitySize = evaluateCellsStructured.ConnectivitySize;
    this->NumberOfOutputCells = evaluateCellsStructured.NumberOfOutputCells;
    this->CellsMap = evaluateCellsStructured.CellsMap;
    this->BatchInfo.BatchSize = evaluateCellsStructured.BatchInfo.BatchSize;
    this->BatchInfo.Batches = std::move(evaluateCellsStructured.BatchInfo.Batches);
    this->Edges = std::move(evaluateCellsStructured.Edges);
  }
};

//-----------------------------------------------------------------------------
// Extract cells structured
template <typename TGrid, typename TPointsArray, typename TInputIdType, typename TOutputIdType>
struct ExtractCellsStructuredFunctor
{
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  using TOutputIdTypeArray = vtkAOSDataArrayTemplate<TOutputIdType>;

  TGrid* Input;
  TPointsArray* InPointsArray;
  double* Origin;
  double* Normal;
  const unsigned char* Selected;
  const unsigned char* InOut;
  const double* Slice;
  bool GeneratePolygons;
  const unsigned int BatchSize;
  bool Interpolate;
  vtkUnsignedCharArray* CellsMap;
  const SliceBatchInfo& BatchInfo;
  ArrayList& CellDataArrays;
  const TEdgeLocator& EdgeLocator;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfEdges;

  int Dimensions[3];
  int CellDimensions[3];
  vtkIdType SliceOffset;
  vtkIdType CellSliceOffset;

  vtkSmartPointer<TOutputIdTypeArray> Connectivity;
  vtkSmartPointer<TOutputIdTypeArray> Offsets;
  vtkSmartPointer<vtkCellArray> OutputCellArray;

  ExtractCellsStructuredFunctor(TGrid* input, TPointsArray* pointsArray, double* origin,
    double* normal, const unsigned char* selected, const unsigned char* inOut, const double* slice,
    bool generatePolygons, unsigned int batchSize, bool interpolate, vtkUnsignedCharArray* cellsMap,
    const SliceBatchInfo& batchInfo, ArrayList& cellDataArrays, const TEdgeLocator& edgeLocator,
    vtkIdType connectivitySize, vtkIdType numberOfOutputCells, vtkIdType numberOfEdges)
    : Input(input)
    , InPointsArray(pointsArray)
    , Origin(origin)
    , Normal(normal)
    , Selected(selected)
    , InOut(inOut)
    , Slice(slice)
    , GeneratePolygons(generatePolygons)
    , BatchSize(batchSize)
    , Interpolate(interpolate)
    , CellsMap(cellsMap)
    , BatchInfo(batchInfo)
    , CellDataArrays(cellDataArrays)
    , EdgeLocator(edgeLocator)
    , ConnectivitySize(connectivitySize)
    , NumberOfOutputCells(numberOfOutputCells)
    , NumberOfEdges(numberOfEdges)
  {
    // create connectivity array, offsets array, and types array
    this->Connectivity = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Connectivity->SetNumberOfValues(this->ConnectivitySize);
    this->Offsets = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Offsets->SetNumberOfValues(this->NumberOfOutputCells + 1);

    TGrid* sgrid = TGrid::SafeDownCast(this->Input);
    sgrid->GetDimensions(this->Dimensions);
    this->CellDimensions[0] = this->Dimensions[0] - 1;
    this->CellDimensions[1] = this->Dimensions[1] - 1;
    this->CellDimensions[2] = this->Dimensions[2] - 1;
    this->SliceOffset = static_cast<vtkIdType>(this->Dimensions[0]) * this->Dimensions[1];
    this->CellSliceOffset =
      static_cast<vtkIdType>(this->CellDimensions[0]) * this->CellDimensions[1];
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto points = vtk::DataArrayTupleRange<3>(this->InPointsArray);
    auto cellsMap = vtk::DataArrayValueRange<1>(this->CellsMap);
    auto connectivity = vtk::DataArrayValueRange<1>(this->Connectivity);
    auto offsets = vtk::DataArrayValueRange<1>(this->Offsets);
    vtkIdType cellId, npts, offset, outputCellId;
    TInputIdType pointIndex1, pointIndex2;
    vtkIdType cellIds[8], newCellIds[12];
    int* edge;
    int caseIndex, point1Index, point2Index, i;
    double s[8];

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      const SliceBatch& batch = this->BatchInfo.Batches[batchId];
      outputCellId = batch.BeginCellsOffsets;
      offset = batch.BeginCellsConnectivity;

      // Traverse this batch of cells (whose bounding sphere possibly
      // intersects the plane).
      for (cellId = batch.BeginCellId; cellId < batch.EndCellId; ++cellId)
      {
        // process cells that have output cells
        if (cellsMap[cellId] == 1)
        {
          ::ComputeCellPointIds(cellId, cellIds, this->CellDimensions, this->Dimensions,
            this->CellSliceOffset, this->SliceOffset);

          if (this->Selected)
          {
            // Get the slice values
            for (i = 0; i < 8; ++i)
            {
              const auto& cellPoint = points[cellIds[i]];
              s[i] = (cellPoint[0] - this->Origin[0]) * this->Normal[0] +
                (cellPoint[1] - this->Origin[1]) * this->Normal[1] +
                (cellPoint[2] - this->Origin[2]) * this->Normal[2];
            }
          }
          else
          {
            // Get the slice values
            for (i = 0; i < 8; ++i)
            {
              s[i] = this->Slice[cellIds[i]];
            }
          }

          // Return if we are not producing anything
          if (::SkipCell(s))
          {
            continue;
          }

          // Build the case table and start producing sn output polygon as necessary
          for (i = 0, caseIndex = 0; i < 8; ++i)
          {
            if (s[i] >= 0.0)
            {
              caseIndex |= CASE_MASK[i];
            }
          }

          edge = ::GetEdge(this->GeneratePolygons, caseIndex);

          // Produce the intersections
          while (*edge > -1) // for all polygons/triangles
          {
            npts = this->GeneratePolygons ? *edge++ : 3;
            offsets[outputCellId] = static_cast<TOutputIdType>(offset);
            // start polygon/triangle edge intersections
            for (i = 0; i < npts; i++, ++edge)
            {
              point1Index = EDGE_CASE[*edge][0];
              point2Index = EDGE_CASE[*edge][1];
              if (point2Index < point1Index)
              {
                std::swap(point1Index, point2Index);
              }

              pointIndex1 = static_cast<TInputIdType>(cellIds[point1Index]);
              pointIndex2 = static_cast<TInputIdType>(cellIds[point2Index]);

              newCellIds[i] = this->EdgeLocator.IsInsertedEdge(pointIndex1, pointIndex2);
              connectivity[offset++] = static_cast<TOutputIdType>(newCellIds[i]);
            } // for all edges of polygon/triangle
            if (this->Interpolate)
            {
              this->CellDataArrays.Copy(cellId, outputCellId);
            }
            ++outputCellId;
          } // for each polygon/triangle
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
template <typename TGrid, typename TInputIdType, typename TOutputIdType>
struct ExtractCellsStructuredWorker
{
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  vtkSmartPointer<vtkCellArray> OutputCellArray;

  template <typename TPointsArray>
  void operator()(TPointsArray* pointsArray, TGrid* input, double* origin, double* normal,
    const unsigned char* selected, const unsigned char* inOut, const double* slice,
    bool generatePolygons, unsigned int batchSize, bool interpolate, vtkUnsignedCharArray* cellsMap,
    const SliceBatchInfo& batchInfo, ArrayList& cellDataArrays, const TEdgeLocator& edgeLocator,
    vtkIdType connectivitySize, vtkIdType numberOfOutputCells, vtkIdType numberOfEdges)
  {
    ExtractCellsStructuredFunctor<TGrid, TPointsArray, TInputIdType, TOutputIdType>
      extractCellsStructured(input, pointsArray, origin, normal, selected, inOut, slice,
        generatePolygons, batchSize, interpolate, cellsMap, batchInfo, cellDataArrays, edgeLocator,
        connectivitySize, numberOfOutputCells, numberOfEdges);
    vtkSMPTools::For(0, static_cast<vtkIdType>(extractCellsStructured.BatchInfo.Batches.size()),
      extractCellsStructured);
    this->OutputCellArray = extractCellsStructured.OutputCellArray;
  }
};

//-----------------------------------------------------------------------------
// Extract points
template <typename TInputIdType>
struct ExtractPointsWorker
{
  using TEdge = EdgeType<TInputIdType>;

  template <typename TInputPoints, typename TOutputPoints>
  void operator()(TInputPoints* inputPoints, TOutputPoints* outputPoints, bool interpolate,
    ArrayList& pointDataArrays, const std::vector<TEdge>& edges, vtkIdType numberOfEdges)
  {
    // create edge points
    auto extractEdgePoints = [&](vtkIdType beginEdgeId, vtkIdType endEdgeId) {
      const auto& inPts = vtk::DataArrayTupleRange<3>(inputPoints);
      auto outPts = vtk::DataArrayTupleRange<3>(outputPoints);

      for (vtkIdType edgeId = beginEdgeId; edgeId < endEdgeId; ++edgeId)
      {
        const TEdge& edge = edges[edgeId];
        const auto edgePoint1 = inPts[edge.V0];
        const auto edgePoint2 = inPts[edge.V1];
        auto outputPoint = outPts[edgeId];

        const double& percentage = edge.Data;
        double bPercentage = 1.0 - percentage;
        outputPoint[0] = edgePoint1[0] * percentage + edgePoint2[0] * bPercentage;
        outputPoint[1] = edgePoint1[1] * percentage + edgePoint2[1] * bPercentage;
        outputPoint[2] = edgePoint1[2] * percentage + edgePoint2[2] * bPercentage;
        if (interpolate)
        {
          pointDataArrays.InterpolateEdge(edge.V0, edge.V1, bPercentage, edgeId);
        }
      }
    };
    vtkSMPTools::For(0, numberOfEdges, extractEdgePoints);
  }
};

//-----------------------------------------------------------------------------
template <typename TGrid, typename TInputIdType>
vtkSmartPointer<vtkPolyData> SliceStructuredData(TGrid* inputGrid, vtkDataArray* pointsArray,
  int outputPointsPrecision, vtkSphereTree* tree, double* origin, double* normal, bool interpolate,
  bool generatePolygons, bool allCellsVisible, unsigned int batchSize)
{
  // Evaluate points or get the selected cells using the sphere-tree
  const unsigned char* selected = nullptr;
  const unsigned char* inOut = nullptr;
  const double* slice = nullptr;
  using DispatcherPoints = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  EvaluatePointsWithPlaneWorker evaluatePointsWorker;
  if (tree)
  {
    vtkIdType numSelected;
    selected = tree->SelectPlane(origin, normal, numSelected);
  }
  else
  {
    if (!DispatcherPoints::Execute(pointsArray, evaluatePointsWorker, origin, normal))
    {
      evaluatePointsWorker(pointsArray, origin, normal);
    }
    inOut = evaluatePointsWorker.InOutArray->GetPointer(0);
    slice = evaluatePointsWorker.SliceArray->GetPointer(0);
  }

  // Evaluate cells and calculate connectivitySize, numberOfOutputCells,
  // batchInfo, cellsMap, edges
  EvaluateCellsStructuredWorker<TGrid, TInputIdType> evaluateCellsStructuredWorker;
  if (!DispatcherPoints::Execute(pointsArray, evaluateCellsStructuredWorker, inputGrid, origin,
        normal, selected, inOut, slice, generatePolygons, allCellsVisible, batchSize))
  {
    evaluateCellsStructuredWorker(pointsArray, inputGrid, origin, normal, selected, inOut, slice,
      generatePolygons, allCellsVisible, batchSize);
  }

  using TEdge = EdgeType<TInputIdType>;
  const vtkIdType connectivitySize = evaluateCellsStructuredWorker.ConnectivitySize;
  const vtkIdType numberOfOutputCells = evaluateCellsStructuredWorker.NumberOfOutputCells;
  const SliceBatchInfo& batchInfo = evaluateCellsStructuredWorker.BatchInfo;
  vtkSmartPointer<vtkUnsignedCharArray> cellsMap = evaluateCellsStructuredWorker.CellsMap;
  std::vector<TEdge> edges = std::move(evaluateCellsStructuredWorker.Edges);

  // Create Edge locator which will be used to define the connectivity of cells
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  TEdgeLocator edgeLocator;
  if (!edges.empty())
  {
    edgeLocator.BuildLocator(static_cast<vtkIdType>(edges.size()), edges.data());
  }
  const TInputIdType numberOfEdges = edgeLocator.GetNumberOfEdges();

  // Calculate total number of output points
  const vtkIdType numberOfOutputPoints = numberOfEdges;

  // Initialize outputPoints
  auto outputPoints = vtkSmartPointer<vtkPoints>::New();
  if (outputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    outputPoints->SetDataType(pointsArray->GetDataType());
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
  if (interpolate)
  {
    outputPointData->InterpolateAllocate(inputGrid->GetPointData(), numberOfOutputPoints);
    pointDataArrays.AddArrays(numberOfOutputPoints, inputGrid->GetPointData(), outputPointData,
      /*nullValue*/ 0.0, /*promote*/ false);
  }
  // define outputCellArray
  vtkSmartPointer<vtkCellArray> outputCellArray;
  // initialize outputCellData
  auto outputCellData = vtkSmartPointer<vtkCellData>::New();
  ArrayList cellDataArrays;
  if (interpolate)
  {
    outputCellData->CopyAllocate(inputGrid->GetCellData(), numberOfOutputCells);
    cellDataArrays.AddArrays(numberOfOutputCells, inputGrid->GetCellData(), outputCellData,
      /*nullValue*/ 0.0, /*promote*/ false);
  }

#ifdef VTK_USE_64BIT_IDS
  bool use64BitsIds = (connectivitySize > VTK_INT_MAX || numberOfOutputPoints > VTK_INT_MAX);
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    // Extract cells and calculate, cell array, cell data
    ExtractCellsStructuredWorker<TGrid, TInputIdType, TOutputIdType> extractCellsStructuredWorker;
    if (!DispatcherPoints::Execute(pointsArray, extractCellsStructuredWorker, inputGrid, origin,
          normal, selected, inOut, slice, generatePolygons, batchSize, interpolate, cellsMap.Get(),
          batchInfo, cellDataArrays, edgeLocator, connectivitySize, numberOfOutputCells,
          numberOfEdges))
    {
      extractCellsStructuredWorker(pointsArray, inputGrid, origin, normal, selected, inOut, slice,
        generatePolygons, batchSize, interpolate, cellsMap.Get(), batchInfo, cellDataArrays,
        edgeLocator, connectivitySize, numberOfOutputCells, numberOfEdges);
    }
    outputCellArray = extractCellsStructuredWorker.OutputCellArray;
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    // Extract cells and calculate, cell array, cell data
    ExtractCellsStructuredWorker<TGrid, TInputIdType, TOutputIdType> extractCellsStructuredWorker;
    if (!DispatcherPoints::Execute(pointsArray, extractCellsStructuredWorker, inputGrid, origin,
          normal, selected, inOut, slice, generatePolygons, batchSize, interpolate, cellsMap.Get(),
          batchInfo, cellDataArrays, edgeLocator, connectivitySize, numberOfOutputCells,
          numberOfEdges))
    {
      extractCellsStructuredWorker(pointsArray, inputGrid, origin, normal, selected, inOut, slice,
        generatePolygons, batchSize, interpolate, cellsMap.Get(), batchInfo, cellDataArrays,
        edgeLocator, connectivitySize, numberOfOutputCells, numberOfEdges);
    }
    outputCellArray = extractCellsStructuredWorker.OutputCellArray;
  }

  // Extract points and calculate outputPoints and outputPointData.
  ExtractPointsWorker<TInputIdType> extractPointsWorker;
  using ExtractPointsDispatch =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!ExtractPointsDispatch::Execute(pointsArray, outputPoints->GetData(), extractPointsWorker,
        interpolate, pointDataArrays, edges, numberOfEdges))
  {
    extractPointsWorker(
      pointsArray, outputPoints->GetData(), interpolate, pointDataArrays, edges, numberOfEdges);
  }

  auto outputSlicedCells = vtkSmartPointer<vtkPolyData>::New();
  outputSlicedCells->SetPoints(outputPoints);
  outputSlicedCells->SetPolys(outputCellArray);
  if (interpolate)
  {
    outputSlicedCells->GetPointData()->ShallowCopy(outputPointData);
    outputSlicedCells->GetCellData()->ShallowCopy(outputCellData);
  }

  return outputSlicedCells;
}
}

//------------------------------------------------------------------------------
// This method drives the various threaded functors to implement the
// plane cutting algorithm.
int vtkStructuredDataPlaneCutter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::GetData(inInfo);
  vtkImageData* inputImage = vtkImageData::GetData(inInfo);
  vtkRectilinearGrid* inputRG = vtkRectilinearGrid::GetData(inInfo);
  vtkStructuredGrid* inputSG = vtkStructuredGrid::GetData(inInfo);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input == nullptr)
  {
    vtkErrorMacro("Input is nullptr");
    return 0;
  }

  // Make sure there is input
  vtkIdType numPts = input->GetNumberOfPoints(), numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
  {
    vtkDebugMacro("No input");
    return 1;
  }

  assert(output != nullptr);
  vtkPlane* plane = this->Plane;
  if (this->Plane == nullptr)
  {
    vtkDebugMacro(<< "Cutting requires vtkPlane");
    return 0;
  }

  bool allCellsVisible = !(input->HasAnyGhostCells() || input->HasAnyBlankCells());

  // Set up the cut operation
  double planeOrigin[3], planeNormal[3];
  plane->GetNormal(planeNormal);
  vtkMath::Normalize(planeNormal);
  plane->GetOrigin(planeOrigin);
  if (plane->GetTransform())
  {
    plane->GetTransform()->TransformNormalAtPoint(planeOrigin, planeNormal, planeNormal);
    plane->GetTransform()->TransformPoint(planeOrigin, planeOrigin);
  }

  if (this->InputInfo.Input != input || this->InputInfo.LastMTime != input->GetMTime())
  {
    if (this->InputInfo.LastMTime != 0)
    {
      this->SetSphereTree(nullptr);
    }
    this->InputInfo = vtkInputInfo(input, input->GetMTime());
  }

  // delegate to flying edges if possible
  if (inputImage && this->GetGeneratePolygons() == 0 && allCellsVisible)
  {
    vtkDataSet* tmpInput = input;
    bool elevationFlag = false;

    // Check to see if there is a scalar associated with the image
    if (!input->GetPointData()->GetScalars())
    {
      // Add an elevation scalar
      vtkNew<vtkElevationFilter> elevation;
      elevation->SetInputData(tmpInput);
      elevation->Update();
      tmpInput = elevation->GetOutput();
      tmpInput->Register(this);
      elevationFlag = true;
    }

    // let flying edges do the work
    vtkNew<vtkFlyingEdgesPlaneCutter> planeCutter;
    vtkNew<vtkPlane> xPlane;
    xPlane->SetOrigin(planeOrigin);
    xPlane->SetNormal(planeNormal);
    planeCutter->SetPlane(xPlane);
    planeCutter->SetComputeNormals(this->ComputeNormals);
    planeCutter->SetInterpolateAttributes(this->InterpolateAttributes);
    planeCutter->SetInputData(tmpInput);
    planeCutter->Update();
    vtkDataSet* slice = planeCutter->GetOutput();
    if (elevationFlag)
    {
      // Remove elevation data
      slice->GetPointData()->RemoveArray("Elevation");
      tmpInput->Delete();
    }
    else if (!this->InterpolateAttributes)
    {
      // Remove unwanted point data
      // In this case, Flying edges outputs only a single array in point data
      // scalars cannot be null
      vtkDataArray* scalars = slice->GetPointData()->GetScalars();
      slice->GetPointData()->RemoveArray(scalars->GetName());
    }
    output->ShallowCopy(slice);
    return 1;
  }

  // build sphere tree if necessary
  if (this->SphereTree == nullptr)
  {
    if (this->BuildTree)
    {
      this->SetSphereTree(vtkSmartPointer<vtkSphereTree>::New());
      this->SphereTree->SetBuildHierarchy(this->BuildHierarchy);
      this->SphereTree->Build(input);
    }
  }
  else
  {
    this->SphereTree->SetBuildHierarchy(this->BuildHierarchy);
    this->SphereTree->Build(input);
  }

  vtkSmartPointer<vtkPolyData> result;
  if (inputImage && (this->GetGeneratePolygons() == 1 || !allCellsVisible))
  {
    int i, j;
    int dataDims[3];
    double spacings[3];
    double tmpValue;
    inputImage->GetDimensions(dataDims);
    inputImage->GetSpacing(spacings);
    const double* dataBBox = inputImage->GetBounds();
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
    rectGrid->GetPointData()->ShallowCopy(inputImage->GetPointData());
    rectGrid->GetCellData()->ShallowCopy(inputImage->GetCellData());
    vtkNew<vtkPoints> points;
    rectGrid->GetPoints(points);
    auto pointsArray = points->GetData();
#ifdef VTK_USE_64BIT_IDS
    bool use64BitsIds = numPts > VTK_INT_MAX;
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      result = SliceStructuredData<vtkRectilinearGrid, TInputIdType>(rectGrid, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      result = SliceStructuredData<vtkRectilinearGrid, TInputIdType>(rectGrid, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize);
    }
  }
  else if (inputSG)
  {
    auto pointsArray = inputSG->GetPoints()->GetData();
#ifdef VTK_USE_64BIT_IDS
    bool use64BitsIds = numPts > VTK_INT_MAX;
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      result = SliceStructuredData<vtkStructuredGrid, TInputIdType>(inputSG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      result = SliceStructuredData<vtkStructuredGrid, TInputIdType>(inputSG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize);
    }
  }
  else // inputRG
  {
    vtkNew<vtkPoints> points;
    inputRG->GetPoints(points);
    auto pointsArray = points->GetData();
#ifdef VTK_USE_64BIT_IDS
    bool use64BitsIds = numPts > VTK_INT_MAX;
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      result = SliceStructuredData<vtkRectilinearGrid, TInputIdType>(inputRG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      result = SliceStructuredData<vtkRectilinearGrid, TInputIdType>(inputRG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize);
    }
  }
  output->ShallowCopy(result);

  // compute normals if requested
  if (this->ComputeNormals)
  {
    vtkNew<vtkFloatArray> newNormals;
    newNormals->SetNumberOfComponents(3);
    newNormals->SetName("Normals");
    newNormals->SetNumberOfTuples(output->GetNumberOfPoints());
    vtkSMPTools::For(0, output->GetNumberOfPoints(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType i = begin; i < end; ++i)
      {
        newNormals->SetTuple(i, planeNormal);
      }
    });
    output->GetPointData()->AddArray(newNormals);
  }

  // Shallow copy field data
  output->GetFieldData()->PassData(input->GetFieldData());

  return 1;
}
