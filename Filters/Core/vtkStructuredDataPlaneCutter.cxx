// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredDataPlaneCutter.h"

#include "vtkAbstractTransform.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkBatch.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkFlyingEdgesPlaneCutter.h"
#include "vtkGarbageCollector.h"
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
#include "vtkStructuredCellArray.h"
#include "vtkStructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
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
//    batchInfo, cellsCase, edges
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
  vtkStructuredDataPlaneCutter* Filter;

  vtkSmartPointer<vtkUnsignedCharArray> InOutArray;
  vtkSmartPointer<vtkDoubleArray> SliceArray;

  EvaluatePointsWithPlaneFunctor(
    TPointsArray* pointsArray, double* origin, double* normal, vtkStructuredDataPlaneCutter* filter)
    : PointsArray(pointsArray)
    , Origin(origin)
    , Normal(normal)
    , Filter(filter)
  {
    this->InOutArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->InOutArray->SetNumberOfValues(this->PointsArray->GetNumberOfTuples());
    this->SliceArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->SliceArray->SetNumberOfValues(this->PointsArray->GetNumberOfTuples());
  }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId)
  {
    const auto& points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    auto inOut = vtk::DataArrayValueRange<1>(this->InOutArray);
    auto slice = vtk::DataArrayValueRange<1>(this->SliceArray);
    static constexpr double zero = double(0);
    double point[3];

    const bool isFirst = vtkSMPTools::GetSingleThread();
    const vtkIdType checkAbortInterval = std::min((endPtId - beginPtId) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType ptId = beginPtId; ptId < endPtId; ++ptId)
    {
      if (ptId % checkAbortInterval == 0)
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
      // GetTuple creates a copy of the tuple using GetTypedTuple if it's not a vktDataArray
      // we do that since the input points can be implicit points, and GetTypedTuple is faster
      // than accessing the component of the TupleReference using GetTypedComponent internally.
      points.GetTuple(ptId, point);

      // Evaluate position of the point with the plane. Invoke inline,
      // non-virtual version of evaluate method.
      slice[ptId] = vtkPlane::Evaluate(this->Normal, this->Origin, point);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      inOut[ptId] = (slice[ptId] > zero ? 2 : (slice[ptId] < zero ? 1 : 0));
    }
  }
};

//-----------------------------------------------------------------------------
struct EvaluatePointsWithPlaneWorker
{
  vtkSmartPointer<vtkUnsignedCharArray> InOutArray;
  vtkSmartPointer<vtkDoubleArray> SliceArray;

  template <typename TPointsArray>
  void operator()(
    TPointsArray* pointsArray, double* origin, double* normal, vtkStructuredDataPlaneCutter* filter)
  {
    EvaluatePointsWithPlaneFunctor<TPointsArray> evaluatePoints(
      pointsArray, origin, normal, filter);
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
struct SliceBatchData
{
  // In EvaluateCells::operator() this is used as an accumulator
  // in EvaluateCells::Reduce() this is changed to an offset
  // This is done to reduce memory footprint.
  vtkIdType CellsOffset;
  vtkIdType CellsConnectivityOffset;

  SliceBatchData()
    : CellsOffset(0)
    , CellsConnectivityOffset(0)
  {
  }
  ~SliceBatchData() = default;
  SliceBatchData& operator+=(const SliceBatchData& other)
  {
    this->CellsOffset += other.CellsOffset;
    this->CellsConnectivityOffset += other.CellsConnectivityOffset;
    return *this;
  }
  SliceBatchData operator+(const SliceBatchData& other) const
  {
    SliceBatchData result = *this;
    result += other;
    return result;
  }
};
using SliceBatch = vtkBatch<SliceBatchData>;
using SliceBatches = vtkBatches<SliceBatchData>;

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
// numberOfCentroids, batchInfo, cellsCase, edges
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

  vtkSmartPointer<vtkStructuredCellArray> StructuredCellArray;
  vtkSMPThreadLocal<std::vector<TEdge>> TLEdges;

  SliceBatches Batches;
  vtkSmartPointer<vtkUnsignedCharArray> CellsCase;
  std::vector<TEdge> Edges;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkStructuredDataPlaneCutter* Filter;

  EvaluateCellsStructuredFunctor(TGrid* input, TPointsArray* pointsArray, double* origin,
    double* normal, const unsigned char* selected, const unsigned char* inOut, const double* slice,
    bool generatePolygons, bool allCellsVisible, unsigned int batchSize,
    vtkStructuredDataPlaneCutter* filter)
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
    , Filter(filter)
  {
    // initialize batches
    this->Batches.Initialize(this->NumberOfInputCells, this->BatchSize);
    // initialize cellsMap
    this->CellsCase = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->CellsCase->SetNumberOfValues(this->NumberOfInputCells);

    if (auto sg = vtkStructuredGrid::SafeDownCast(input))
    {
      this->StructuredCellArray = sg->GetCells();
    }
    else
    {
      // we want deal with voxels as hexahedrons
      int extent[6];
      input->GetExtent(extent);
      this->StructuredCellArray = vtkStructuredData::GetCellArray(extent, false);
    }
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
    auto cellsCase = vtk::DataArrayValueRange<1>(this->CellsCase);
    vtkIdType cellId, npts, numberOfCells, cellsConnectivitySize;
    TInputIdType pointIndex1, pointIndex2;
    vtkIdType cellSize, cellIds[8];
    double cellPoint[3];
    int* edge;
    int caseIndex, point1Index, point2Index, i;
    double s[8], point1ToPoint2, point1ToIso, point1Weight;
    bool needCell;
    const auto cells = this->StructuredCellArray;

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
      SliceBatch& batch = this->Batches[batchId];
      auto& batchNumberOfCells = batch.Data.CellsOffset;
      auto& batchCellsConnectivity = batch.Data.CellsConnectivityOffset;

      const unsigned char* selected = nullptr;
      if (this->Selected)
      {
        selected = this->Selected + batch.BeginId;
      }
      // Traverse this batch of cells (whose bounding sphere possibly
      // intersects the plane).
      for (cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
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
            cells->GetCellAtId(cellId, cellSize, cellIds);

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
        caseIndex = 0;
        if (needCell)
        {
          if (this->Selected)
          {
            cells->GetCellAtId(cellId, cellSize, cellIds);

            // Get the slice values
            for (i = 0; i < 8; ++i)
            {
              // GetTuple creates a copy of the tuple using GetTypedTuple if it's not a vktDataArray
              // we do that since the input points can be implicit points, and GetTypedTuple is
              // faster than accessing the component of the TupleReference using GetTypedComponent
              // internally.
              points.GetTuple(cellIds[i], cellPoint);
              s[i] = vtkPlane::Evaluate(this->Normal, this->Origin, cellPoint);
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
            cellsCase[cellId] = 0;
            continue;
          }

          // Build the case table and start producing sn output polygon as necessary
          for (i = 0; i < 8; ++i)
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
        batchNumberOfCells += numberOfCells;
        batchCellsConnectivity += cellsConnectivitySize;
        cellsCase[cellId] = static_cast<unsigned char>(caseIndex);
      }
    }
  }

  void Reduce()
  {
    // trim batches with 0 cells in-place
    this->Batches.TrimBatches([](const SliceBatch& batch) { return batch.Data.CellsOffset == 0; });

    // assign BeginCellsOffsets/BeginCellsConnectivity for each batch
    const auto globalSum = this->Batches.BuildOffsetsAndGetGlobalSum();
    this->NumberOfOutputCells = globalSum.CellsOffset;
    this->ConnectivitySize = globalSum.CellsConnectivityOffset;

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
    vtkSMPTools::For(0, static_cast<vtkIdType>(tlEdgesVector.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
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
  SliceBatches Batches;
  vtkSmartPointer<vtkUnsignedCharArray> CellsCase;
  using TEdge = EdgeType<TInputIdType>;
  std::vector<TEdge> Edges;

  template <typename TPointsArray>
  void operator()(TPointsArray* pointsArray, TGrid* input, double* origin, double* normal,
    const unsigned char* selected, const unsigned char* inOut, const double* slice,
    bool generatePolygons, bool allCellsVisible, unsigned int batchSize,
    vtkStructuredDataPlaneCutter* filter)
  {
    EvaluateCellsStructuredFunctor<TGrid, TPointsArray, TInputIdType> evaluateCellsStructured(input,
      pointsArray, origin, normal, selected, inOut, slice, generatePolygons, allCellsVisible,
      batchSize, filter);
    vtkSMPTools::For(
      0, evaluateCellsStructured.Batches.GetNumberOfBatches(), evaluateCellsStructured);
    this->ConnectivitySize = evaluateCellsStructured.ConnectivitySize;
    this->NumberOfOutputCells = evaluateCellsStructured.NumberOfOutputCells;
    this->CellsCase = evaluateCellsStructured.CellsCase;
    this->Batches = std::move(evaluateCellsStructured.Batches);
    this->Edges = std::move(evaluateCellsStructured.Edges);
  }
};

//-----------------------------------------------------------------------------
// Extract cells structured
template <typename TGrid, typename TInputIdType, typename TOutputIdType>
struct ExtractCellsStructuredFunctor
{
  using TEdgeLocator = EdgeLocatorType<TInputIdType>;
  using TOutputIdTypeArray = vtkAOSDataArrayTemplate<TOutputIdType>;

  TGrid* Input;
  bool GeneratePolygons;
  bool Interpolate;
  vtkUnsignedCharArray* CellsCase;
  const SliceBatches& Batches;
  ArrayList& CellDataArrays;
  const TEdgeLocator& EdgeLocator;
  vtkIdType ConnectivitySize;
  vtkIdType NumberOfOutputCells;
  vtkIdType NumberOfEdges;
  vtkStructuredDataPlaneCutter* Filter;

  vtkSmartPointer<vtkStructuredCellArray> StructuredCellArray;

  vtkSmartPointer<TOutputIdTypeArray> Connectivity;
  vtkSmartPointer<TOutputIdTypeArray> Offsets;
  vtkSmartPointer<vtkCellArray> OutputCellArray;

  ExtractCellsStructuredFunctor(TGrid* input, bool generatePolygons, bool interpolate,
    vtkUnsignedCharArray* cellsCase, const SliceBatches& batches, ArrayList& cellDataArrays,
    const TEdgeLocator& edgeLocator, vtkIdType connectivitySize, vtkIdType numberOfOutputCells,
    vtkIdType numberOfEdges, vtkStructuredDataPlaneCutter* filter)
    : Input(input)
    , GeneratePolygons(generatePolygons)
    , Interpolate(interpolate)
    , CellsCase(cellsCase)
    , Batches(batches)
    , CellDataArrays(cellDataArrays)
    , EdgeLocator(edgeLocator)
    , ConnectivitySize(connectivitySize)
    , NumberOfOutputCells(numberOfOutputCells)
    , NumberOfEdges(numberOfEdges)
    , Filter(filter)
  {
    // create connectivity array, offsets array, and types array
    this->Connectivity = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Connectivity->SetNumberOfValues(this->ConnectivitySize);
    this->Offsets = vtkSmartPointer<TOutputIdTypeArray>::New();
    this->Offsets->SetNumberOfValues(this->NumberOfOutputCells + 1);

    if (auto sg = vtkStructuredGrid::SafeDownCast(input))
    {
      this->StructuredCellArray = sg->GetCells();
    }
    else
    {
      // we want deal with voxels as hexahedrons
      int extent[6];
      input->GetExtent(extent);
      this->StructuredCellArray = vtkStructuredData::GetCellArray(extent, false);
    }
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto cellsCase = vtk::DataArrayValueRange<1>(this->CellsCase);
    auto connectivity = vtk::DataArrayValueRange<1>(this->Connectivity);
    auto offsets = vtk::DataArrayValueRange<1>(this->Offsets);
    vtkIdType cellId, npts;
    TInputIdType pointIndex1, pointIndex2;
    vtkIdType cellSize, cellIds[8], newCellIds[12];
    int* edge;
    int caseIndex, point1Index, point2Index, i;
    bool processCell;
    const auto cells = this->StructuredCellArray;

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
      const SliceBatch& batch = this->Batches[batchId];
      auto cellsOffset = batch.Data.CellsOffset;
      auto cellsConnectivityOffset = batch.Data.CellsConnectivityOffset;

      // Traverse this batch of cells (whose bounding sphere possibly
      // intersects the plane).
      for (cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
      {
        caseIndex = static_cast<int>(cellsCase[cellId]);
        processCell = caseIndex != 0 && caseIndex != 255;
        // process cells that have output cells
        if (processCell)
        {
          cells->GetCellAtId(cellId, cellSize, cellIds);

          edge = ::GetEdge(this->GeneratePolygons, caseIndex);

          // Produce the intersections
          while (*edge > -1) // for all polygons/triangles
          {
            npts = this->GeneratePolygons ? *edge++ : 3;
            offsets[cellsOffset] = static_cast<TOutputIdType>(cellsConnectivityOffset);
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
              connectivity[cellsConnectivityOffset++] = static_cast<TOutputIdType>(newCellIds[i]);
            } // for all edges of polygon/triangle
            if (this->Interpolate)
            {
              this->CellDataArrays.Copy(cellId, cellsOffset);
            }
            ++cellsOffset;
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
// Extract points
template <typename TInputIdType>
struct ExtractPointsWorker
{
  using TEdge = EdgeType<TInputIdType>;

  template <typename TInputPoints, typename TOutputPoints>
  void operator()(TInputPoints* inputPoints, TOutputPoints* outputPoints, bool interpolate,
    ArrayList& pointDataArrays, const std::vector<TEdge>& edges, vtkIdType numberOfEdges,
    vtkStructuredDataPlaneCutter* filter)
  {
    // create edge points
    auto extractEdgePoints = [&](vtkIdType beginEdgeId, vtkIdType endEdgeId)
    {
      const auto& inPts = vtk::DataArrayTupleRange<3>(inputPoints);
      auto outPts = vtk::DataArrayTupleRange<3>(outputPoints);
      double edgePoint1[3], edgePoint2[3];

      const bool isFirst = vtkSMPTools::GetSingleThread();
      const vtkIdType checkAbortInterval =
        std::min((endEdgeId - beginEdgeId) / 10 + 1, (vtkIdType)1000);
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
        auto outputPoint = outPts[edgeId];

        const double& percentage = edge.Data;
        const double bPercentage = 1.0 - percentage;
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
  bool generatePolygons, bool allCellsVisible, unsigned int batchSize,
  vtkStructuredDataPlaneCutter* filter)
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
    if (!DispatcherPoints::Execute(pointsArray, evaluatePointsWorker, origin, normal, filter))
    {
      evaluatePointsWorker(pointsArray, origin, normal, filter);
    }
    inOut = evaluatePointsWorker.InOutArray->GetPointer(0);
    slice = evaluatePointsWorker.SliceArray->GetPointer(0);
  }

  // Evaluate cells and calculate connectivitySize, numberOfOutputCells,
  // batchInfo, cellsMap, edges
  EvaluateCellsStructuredWorker<TGrid, TInputIdType> evaluateCellsStructuredWorker;
  if (!DispatcherPoints::Execute(pointsArray, evaluateCellsStructuredWorker, inputGrid, origin,
        normal, selected, inOut, slice, generatePolygons, allCellsVisible, batchSize, filter))
  {
    evaluateCellsStructuredWorker(pointsArray, inputGrid, origin, normal, selected, inOut, slice,
      generatePolygons, allCellsVisible, batchSize, filter);
  }

  using TEdge = EdgeType<TInputIdType>;
  const vtkIdType connectivitySize = evaluateCellsStructuredWorker.ConnectivitySize;
  const vtkIdType numberOfOutputCells = evaluateCellsStructuredWorker.NumberOfOutputCells;
  const SliceBatches& batches = evaluateCellsStructuredWorker.Batches;
  vtkSmartPointer<vtkUnsignedCharArray> cellsCase = evaluateCellsStructuredWorker.CellsCase;
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
  const bool use64BitsIds = (connectivitySize > VTK_INT_MAX || numberOfOutputPoints > VTK_INT_MAX);
  if (use64BitsIds)
  {
    using TOutputIdType = vtkTypeInt64;
    // Extract cells and calculate, cell array, cell data
    ExtractCellsStructuredFunctor<TGrid, TInputIdType, TOutputIdType> extractCellsStructured(
      inputGrid, generatePolygons, interpolate, cellsCase, batches, cellDataArrays, edgeLocator,
      connectivitySize, numberOfOutputCells, numberOfEdges, filter);
    vtkSMPTools::For(
      0, extractCellsStructured.Batches.GetNumberOfBatches(), extractCellsStructured);
    outputCellArray = extractCellsStructured.OutputCellArray;
  }
  else
#endif
  {
    using TOutputIdType = vtkTypeInt32;
    // Extract cells and calculate, cell array, cell data
    ExtractCellsStructuredFunctor<TGrid, TInputIdType, TOutputIdType> extractCellsStructured(
      inputGrid, generatePolygons, interpolate, cellsCase, batches, cellDataArrays, edgeLocator,
      connectivitySize, numberOfOutputCells, numberOfEdges, filter);
    vtkSMPTools::For(
      0, extractCellsStructured.Batches.GetNumberOfBatches(), extractCellsStructured);
    outputCellArray = extractCellsStructured.OutputCellArray;
  }

  // Extract points and calculate outputPoints and outputPointData.
  ExtractPointsWorker<TInputIdType> extractPointsWorker;
  using ExtractPointsDispatch =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  if (!ExtractPointsDispatch::Execute(pointsArray, outputPoints->GetData(), extractPointsWorker,
        interpolate, pointDataArrays, edges, numberOfEdges, filter))
  {
    extractPointsWorker(pointsArray, outputPoints->GetData(), interpolate, pointDataArrays, edges,
      numberOfEdges, filter);
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

  static constexpr int MASKED_CELL_VALUE = vtkDataSetAttributes::DUPLICATECELL |
    vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;
  const bool hasCellGhosts = input->GetCellData()->HasAnyGhostBitSet(MASKED_CELL_VALUE);
  const bool hasPointGhosts = input->HasAnyBlankPoints();
  const bool allCellsVisible = !(hasCellGhosts || hasPointGhosts);
  const bool hasOnlyCellGhosts = hasCellGhosts && !hasPointGhosts;

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
  const bool canHandleGhostsIfPresent =
    allCellsVisible || (hasOnlyCellGhosts && this->InterpolateAttributes);
  if (inputImage && this->GetGeneratePolygons() == 0 && canHandleGhostsIfPresent)
  {
    vtkDataSet* tmpInput = input;
    bool addScalars = false;

    // Check to see if there is a scalar associated with the image
    auto scalars = input->GetPointData()->GetScalars();
    if (!scalars || scalars->GetNumberOfComponents() != 1)
    {
      auto tmpImage = vtkSmartPointer<vtkImageData>::New();
      tmpImage->ShallowCopy(inputImage);
      // Add a scalar to the image
      vtkNew<vtkFloatArray> constantScalars;
      constantScalars->SetName("ConstantScalars");
      constantScalars->SetNumberOfComponents(1);
      constantScalars->SetNumberOfTuples(tmpImage->GetNumberOfPoints());
      vtkSMPTools::For(0, tmpImage->GetNumberOfPoints(),
        [&](vtkIdType begin, vtkIdType end)
        { std::fill_n(constantScalars->GetPointer(begin), (end - begin), 1.0f); });
      tmpImage->GetPointData()->AddArray(constantScalars);
      tmpImage->GetPointData()->SetActiveScalars(constantScalars->GetName());
      tmpInput = tmpImage;
      tmpInput->Register(this);
      addScalars = true;
    }

    // let flying edges do the work
    vtkNew<vtkFlyingEdgesPlaneCutter> planeCutter;
    vtkNew<vtkPlane> xPlane;
    xPlane->SetOrigin(planeOrigin);
    xPlane->SetNormal(planeNormal);
    planeCutter->SetPlane(xPlane);
    planeCutter->SetComputeNormals(this->ComputeNormals);
    planeCutter->SetInterpolateAttributes(this->InterpolateAttributes);
    planeCutter->SetContainerAlgorithm(this);
    planeCutter->SetInputData(tmpInput);
    planeCutter->Update();
    vtkPolyData* slice = planeCutter->GetOutput();
    if (addScalars)
    {
      // Remove scalars data
      slice->GetPointData()->RemoveArray("ConstantScalars");
      tmpInput->Delete();
      if (scalars && scalars->GetNumberOfComponents() != 1)
      {
        slice->GetPointData()->SetActiveScalars(scalars->GetName());
      }
    }
    else if (!this->InterpolateAttributes)
    {
      // Remove unwanted point data
      // In this case, Flying edges outputs only a single array in point data
      // scalars cannot be null
      slice->GetPointData()->RemoveArray(slice->GetPointData()->GetScalars()->GetName());
    }
    // vtkFlyingEdgesPlaneCutter does not handle ghost cells, if there are any ghost cells
    // in the input, we need to remove them from the output
    if (!allCellsVisible)
    {
      slice->RemoveGhostCells();
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
  if (inputImage && (this->GetGeneratePolygons() == 1 || !canHandleGhostsIfPresent))
  {
    auto pointsArray = inputImage->GetPoints()->GetData();
#ifdef VTK_USE_64BIT_IDS
    const bool use64BitsIds = numPts > VTK_INT_MAX;
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      result = SliceStructuredData<vtkImageData, TInputIdType>(inputImage, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize,
        this);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      result = SliceStructuredData<vtkImageData, TInputIdType>(inputImage, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize,
        this);
    }
  }
  else if (inputSG)
  {
    auto pointsArray = inputSG->GetPoints()->GetData();
#ifdef VTK_USE_64BIT_IDS
    const bool use64BitsIds = numPts > VTK_INT_MAX;
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      result = SliceStructuredData<vtkStructuredGrid, TInputIdType>(inputSG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize,
        this);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      result = SliceStructuredData<vtkStructuredGrid, TInputIdType>(inputSG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize,
        this);
    }
  }
  else // inputRG
  {
    auto pointsArray = inputRG->GetPoints()->GetData();
#ifdef VTK_USE_64BIT_IDS
    const bool use64BitsIds = numPts > VTK_INT_MAX;
    if (use64BitsIds)
    {
      using TInputIdType = vtkTypeInt64;
      result = SliceStructuredData<vtkRectilinearGrid, TInputIdType>(inputRG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize,
        this);
    }
    else
#endif
    {
      using TInputIdType = vtkTypeInt32;
      result = SliceStructuredData<vtkRectilinearGrid, TInputIdType>(inputRG, pointsArray,
        this->OutputPointsPrecision, this->SphereTree, planeOrigin, planeNormal,
        this->InterpolateAttributes, this->GeneratePolygons, allCellsVisible, this->BatchSize,
        this);
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
    vtkSMPTools::For(0, output->GetNumberOfPoints(),
      [&](vtkIdType begin, vtkIdType end)
      {
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

//------------------------------------------------------------------------------
void vtkStructuredDataPlaneCutter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // the SphereTree shares our input and can be part of a reference loop
  vtkGarbageCollectorReport(collector, this->SphereTree, "SphereTree");
}
VTK_ABI_NAMESPACE_END
