// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk3DLinearGridCrinkleExtractor.h"

#include "vtk3DLinearGridInternal.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtk3DLinearGridCrinkleExtractor);
vtkCxxSetObjectMacro(vtk3DLinearGridCrinkleExtractor, ImplicitFunction, vtkImplicitFunction);

//------------------------------------------------------------------------------
// Macros immediately below are just used to make code easier to
// read. Invokes functor _op _num times depending on serial (_seq==1) or
// parallel processing mode. The _REDUCE_ version is used to called functors
// with a Reduce() method).
#define EXECUTE_SMPFOR(_seq, _num, _op)                                                            \
  do                                                                                               \
  {                                                                                                \
    if (!_seq)                                                                                     \
    {                                                                                              \
      vtkSMPTools::For(0, _num, _op);                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _op(0, _num);                                                                                \
    }                                                                                              \
  } while (false)

#define EXECUTE_REDUCED_SMPFOR(_seq, _num, _op, _nt)                                               \
  do                                                                                               \
  {                                                                                                \
    if (!_seq)                                                                                     \
    {                                                                                              \
      vtkSMPTools::For(0, _num, _op);                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      _op.Initialize();                                                                            \
      _op(0, _num);                                                                                \
      _op.Reduce();                                                                                \
    }                                                                                              \
    _nt = _op.NumThreadsUsed;                                                                      \
  } while (false)

namespace
{ // anonymous

//========================= Quick implicit function cell selection =============

// Compute an array that classifies each point with respect to the current
// implicit function (i.e. above the function(=2), below the function(=1), on
// the function(=0)).  InOutArray is allocated here and should be deleted by
// the invoking code. InOutArray is an unsigned char array to simplify bit
// fiddling later on (i.e., Intersects() method).
//
// The reason we compute this unsigned char array as compared to an array of
// function values is to reduce the amount of memory used, and writing to
// memory, since these are significant costs for large data.

// Templated for explicit point representations of real type
template <typename TP>
struct FunctionClassifyPoints;

// General classification capability
template <typename TPointArray, typename TImplicitFunction>
struct ClassifyPointsFunctor
{
  TPointArray* Points;
  TImplicitFunction* ImplicitFunction;
  vtkAOSDataArrayTemplate<unsigned char>* InOutArray;
  vtk3DLinearGridCrinkleExtractor* Filter;

  ClassifyPointsFunctor(TPointArray* points, TImplicitFunction* implicitFunction,
    vtkAOSDataArrayTemplate<unsigned char>* inoutArray, vtk3DLinearGridCrinkleExtractor* filter)
    : Points(points)
    , ImplicitFunction(implicitFunction)
    , InOutArray(inoutArray)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double p[3], zero = double(0), eval;
    auto pts = vtk::DataArrayTupleRange<3>(this->Points, ptId, endPtId).begin();
    unsigned char* ioa = this->InOutArray->GetPointer(ptId);
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);
    for (; ptId < endPtId; ++ptId, ++pts)
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
      // Access each point
      pts->GetTuple(p);

      eval = this->ImplicitFunction->EvaluateFunction(p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
      *ioa++ = (eval > zero ? 2 : (eval < zero ? 1 : 0));
    }
  }
};

template <typename TImplicitFunction>
struct ClassifyPointsWorker
{
  template <typename TPointArray>
  void operator()(TPointArray* points, TImplicitFunction* implicitFunction,
    vtkAOSDataArrayTemplate<unsigned char>* inoutArray, vtk3DLinearGridCrinkleExtractor* filter)
  {
    ClassifyPointsFunctor<TPointArray, TImplicitFunction> functor(
      points, implicitFunction, inoutArray, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), points->GetNumberOfTuples(), functor);
  }
};

// Base class for extracting cells and points from the input
// vtkUnstructuredGrid.
struct ExtractCellsBase
{
  typedef std::vector<vtkIdType> CellArrayType;
  typedef std::vector<vtkIdType> OriginCellType;
  typedef std::vector<unsigned char> CellTypesType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    CellArrayType LocalCells;
    OriginCellType LocalOrigins;
    CellTypesType LocalTypes;
    vtkIdType LocalNumCells;
    CellIter LocalCellIter;

    LocalDataType()
      : LocalNumCells(0)
    {
    }
  };

  const unsigned char* InOut;
  CellIter* Iter;
  vtkIdType InputNumPts;
  vtkIdType OutputNumPts;
  vtkIdType OutputNumCells;
  vtkIdType TotalSize;
  vtkUnstructuredGrid* Grid;
  vtkCellArray* Cells;
  bool CopyPointData;
  bool CopyCellData;
  vtkIdType* PointMap;
  vtkIdType* CellMap;
  int NumThreadsUsed;
  vtkSMPThreadLocal<LocalDataType> LocalData;
  vtk3DLinearGridCrinkleExtractor* Filter;

  ExtractCellsBase(vtkIdType inNumPts, CellIter* c, unsigned char* inout, vtkUnstructuredGrid* grid,
    vtkCellArray* cells, vtk3DLinearGridCrinkleExtractor* filter)
    : InOut(inout)
    , Iter(c)
    , InputNumPts(inNumPts)
    , OutputNumPts(0)
    , OutputNumCells(0)
    , TotalSize(0)
    , Grid(grid)
    , Cells(cells)
    , CopyPointData(filter->GetCopyPointData())
    , CopyCellData(filter->GetCopyCellData())
    , PointMap(nullptr)
    , CellMap(nullptr)
    , NumThreadsUsed(0)
    , Filter(filter)
  {
  }

  // Check if a list of points intersects the plane
  static bool Intersects(const unsigned char* inout, vtkIdType npts, const vtkIdType* pts)
  {
    unsigned char onOneSideOfPlane = inout[pts[0]];
    for (vtkIdType i = 1; onOneSideOfPlane && i < npts; ++i)
    {
      onOneSideOfPlane &= inout[pts[i]];
    }
    return (!onOneSideOfPlane);
  }

  // Set up the iteration process
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

}; // ExtractCellsBase

// Traverse all cells and extract intersected cells
struct ExtractCells : public ExtractCellsBase
{
  ExtractCells(vtkIdType inNumPts, CellIter* c, unsigned char* inout, vtkUnstructuredGrid* grid,
    vtkCellArray* cells, vtk3DLinearGridCrinkleExtractor* filter)
    : ExtractCellsBase(inNumPts, c, inout, grid, cells, filter)
  {
  }

  void Initialize() { this->ExtractCellsBase::Initialize(); }

  // operator() method extracts cells
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lCells = localData.LocalCells;
    auto& lOrigins = localData.LocalOrigins;
    auto& lTypes = localData.LocalTypes;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    const unsigned char* inout = this->InOut;
    vtkIdType& lNumCells = localData.LocalNumCells;
    vtkIdType npts;

    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);

    for (; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      // Does the implicit function cut this cell?
      npts = cellIter->NumVerts;
      if (ExtractCellsBase::Intersects(inout, npts, c))
      {
        ++lNumCells;
        lTypes.emplace_back(cellIter->GetCellType(cellId));
        lCells.emplace_back(npts);
        const vtkIdType* pts = cellIter->GetCellIds(cellId);
        for (auto i = 0; i < npts; ++i)
        {
          lCells.emplace_back(pts[i]);
        }
        if (this->CopyCellData)
        {
          lOrigins.emplace_back(cellId); // to support cell data copying
        }
      }                     // if implicit function intersects
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite local thread data. Basically build the output unstructured grid.
  void Reduce()
  {
    // Count the number of cells, and the number of threads used. Figure out the total
    // length of the cell array.
    vtkIdType numCells = 0;
    vtkIdType size = 0;
    for (const auto& threadData : this->LocalData)
    {
      numCells += threadData.LocalNumCells;
      size += static_cast<vtkIdType>(threadData.LocalCells.size());
      this->NumThreadsUsed++;
    }
    this->OutputNumCells = numCells;
    this->TotalSize = size;

    // Now allocate the cell array, offset array, and cell types array.
    this->Cells->AllocateExact(numCells, size - numCells);
    vtkNew<vtkUnsignedCharArray> cellTypes;
    unsigned char* ctptr = static_cast<unsigned char*>(cellTypes->WriteVoidPointer(0, numCells));

    // If cell data is requested, roll up generating cell ids
    vtkIdType* cellMap = nullptr;
    if (this->CopyCellData)
    {
      this->CellMap = cellMap = new vtkIdType[numCells];
    }

    // Now composite the cell-related information
    for (const auto& threadData : this->LocalData)
    {
      const auto& lCells = threadData.LocalCells;
      const auto& lTypes = threadData.LocalTypes;
      const auto& lOrigins = threadData.LocalOrigins;
      numCells = threadData.LocalNumCells;

      this->Cells->AppendLegacyFormat(lCells.data(), static_cast<vtkIdType>(lCells.size()));
      ctptr = std::copy_n(lTypes.cbegin(), numCells, ctptr);
      if (this->CopyCellData)
      {
        cellMap = std::copy_n(lOrigins.cbegin(), numCells, cellMap);
      }
    } // for this thread

    // Define the grid
    this->Grid->SetCells(cellTypes, this->Cells);

  } // Reduce

}; // ExtractCells

// Traverse all cells to extract intersected cells and remapped points
struct ExtractPointsAndCells : public ExtractCellsBase
{
  ExtractPointsAndCells(vtkIdType inNumPts, CellIter* c, unsigned char* inout,
    vtkUnstructuredGrid* grid, vtkCellArray* cells, vtk3DLinearGridCrinkleExtractor* filter)
    : ExtractCellsBase(inNumPts, c, inout, grid, cells, filter)
  {
    this->PointMap = new vtkIdType[inNumPts];
    std::fill_n(this->PointMap, inNumPts, (-1));
  }

  void Initialize() { this->ExtractCellsBase::Initialize(); }

  // operator() method identifies cells and points to extract
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lCells = localData.LocalCells;
    auto& lOrigins = localData.LocalOrigins;
    auto& lTypes = localData.LocalTypes;
    CellIter* cellIter = &localData.LocalCellIter;
    const vtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    const unsigned char* inout = this->InOut;
    vtkIdType& lNumCells = localData.LocalNumCells;
    vtkIdType npts;
    vtkIdType* pointMap = this->PointMap;

    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);

    for (; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      // Does the implicit function cut this cell?
      npts = cellIter->NumVerts;
      if (ExtractCellsBase::Intersects(inout, npts, c))
      {
        ++lNumCells;
        lTypes.emplace_back(cellIter->GetCellType(cellId));
        lCells.emplace_back(npts);
        const vtkIdType* pts = cellIter->GetCellIds(cellId);
        for (auto i = 0; i < npts; ++i)
        {
          pointMap[pts[i]] = 1; // this point is used
          lCells.emplace_back(pts[i]);
        }
        if (this->CopyCellData)
        {
          lOrigins.emplace_back(cellId); // to support cell data copying
        }
      }                     // if implicit function intersects
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite local thread data. Basically build the output unstructured grid.
  void Reduce()
  {
    // Generate point map
    vtkIdType globalPtId = 0;
    vtkIdType* ptMap = this->PointMap;
    for (auto ptId = 0; ptId < this->InputNumPts; ++ptId)
    {
      if (this->PointMap[ptId] > 0)
      {
        ptMap[ptId] = globalPtId++;
      }
    }
    this->OutputNumPts = globalPtId;

    // Count the number of cells, and the number of threads used. Figure out the total
    // length of the cell array.
    vtkIdType numCells = 0;
    vtkIdType size = 0;
    for (const auto& threadData : this->LocalData)
    {
      numCells += threadData.LocalNumCells;
      size += static_cast<vtkIdType>(threadData.LocalCells.size());
      this->NumThreadsUsed++;
    }
    this->OutputNumCells = numCells;
    this->TotalSize = size;

    // Now allocate the cell array, offset array, and cell types array.
    this->Cells->AllocateExact(numCells, size - numCells);
    vtkNew<vtkUnsignedCharArray> cellTypes;
    unsigned char* ctptr = static_cast<unsigned char*>(cellTypes->WriteVoidPointer(0, numCells));

    // If cell data is requested, roll up generating cell ids
    vtkIdType* cellMap = nullptr;
    this->CellMap = cellMap = new vtkIdType[numCells];

    // Now composite the cell-related information
    for (const auto& threadData : this->LocalData)
    {
      const auto& lCells = threadData.LocalCells;
      const auto& lTypes = threadData.LocalTypes;
      const auto& lOrigins = threadData.LocalOrigins;
      numCells = threadData.LocalNumCells;

      ctptr = std::copy_n(lTypes.cbegin(), numCells, ctptr);
      if (this->CopyCellData)
      {
        cellMap = std::copy_n(lOrigins.cbegin(), numCells, cellMap);
      }

      // Need to do this in a loop since the pointIds are mapped through ptMap:
      auto threadCells = lCells.cbegin();
      for (auto i = 0; i < numCells; ++i)
      {
        const vtkIdType npts = *threadCells++;
        this->Cells->InsertNextCell(static_cast<int>(npts));
        for (auto j = 0; j < npts; ++j)
        {
          this->Cells->InsertCellPoint(ptMap[*threadCells++]);
        }
      } // over all the cells in this thread
    }   // for this thread

    // Define the grid
    this->Grid->SetCells(cellTypes, this->Cells);

  } // Reduce

}; // ExtractPointsAndCells

// Copy cell data from input to output
struct CopyCellAttributes
{
  ArrayList* Arrays;
  const vtkIdType* CellMap;
  vtk3DLinearGridCrinkleExtractor* Filter;

  CopyCellAttributes(
    ArrayList* arrays, const vtkIdType* cellMap, vtk3DLinearGridCrinkleExtractor* filter)
    : Arrays(arrays)
    , CellMap(cellMap)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);
    for (; cellId < endCellId; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
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
      this->Arrays->Copy(this->CellMap[cellId], cellId);
    }
  }
};

// Generate point coordinates
template <typename TPArrayIn, typename TPArrayOut>
struct GeneratePointsFunctor
{
  TPArrayIn* InPts;
  TPArrayOut* OutPts;
  const vtkIdType* PointMap;
  vtk3DLinearGridCrinkleExtractor* Filter;
  using TPIn = vtk::GetAPIType<TPArrayIn>;
  using TPOut = vtk::GetAPIType<TPArrayOut>;

  GeneratePointsFunctor(
    TPArrayIn* inPts, TPArrayOut* outPts, vtkIdType* ptMap, vtk3DLinearGridCrinkleExtractor* filter)
    : InPts(inPts)
    , OutPts(outPts)
    , PointMap(ptMap)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto p = vtk::DataArrayTupleRange<3>(this->InPts, ptId, endPtId).begin();
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPts);
    const vtkIdType* ptMap = this->PointMap;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

    for (; ptId < endPtId; ++ptId, ++p)
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

      if (ptMap[ptId] >= 0)
      {
        auto x = outPts[ptMap[ptId]];
        x[0] = static_cast<TPOut>((*p)[0]);
        x[1] = static_cast<TPOut>((*p)[1]);
        x[2] = static_cast<TPOut>((*p)[2]);
      }
    }
  }
};

struct GeneratePointsWorker
{
  template <typename TPArrayIn, typename TPArrayOut>
  void operator()(
    TPArrayIn* inPts, TPArrayOut* outPts, vtkIdType* ptMap, vtk3DLinearGridCrinkleExtractor* filter)
  {
    GeneratePointsFunctor<TPArrayIn, TPArrayOut> functor(inPts, outPts, ptMap, filter);
    EXECUTE_SMPFOR(filter->GetSequentialProcessing(), inPts->GetNumberOfTuples(), functor);
  }
};

// Copy point data from input to output.
struct CopyPointAttributes
{
  ArrayList* Arrays;
  const vtkIdType* PointMap;
  vtk3DLinearGridCrinkleExtractor* Filter;

  CopyPointAttributes(
    ArrayList* arrays, const vtkIdType* ptMap, vtk3DLinearGridCrinkleExtractor* filter)
    : Arrays(arrays)
    , PointMap(ptMap)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const vtkIdType* ptMap = this->PointMap;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);
    for (; ptId < endPtId; ++ptId)
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

      if (ptMap[ptId] >= 0)
      {
        this->Arrays->Copy(ptId, ptMap[ptId]);
      }
    }
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
// Construct an instance of the class.
vtk3DLinearGridCrinkleExtractor::vtk3DLinearGridCrinkleExtractor()
{
  this->ImplicitFunction = nullptr;
  this->CopyPointData = true;
  this->CopyCellData = false;
  this->RemoveUnusedPoints = false;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
}

//------------------------------------------------------------------------------
vtk3DLinearGridCrinkleExtractor::~vtk3DLinearGridCrinkleExtractor()
{
  this->SetImplicitFunction(nullptr);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If the implicit function
// definition is modified, then this object is modified as well.
vtkMTimeType vtk3DLinearGridCrinkleExtractor::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->ImplicitFunction != nullptr)
  {
    vtkMTimeType mTime2 = this->ImplicitFunction->GetMTime();
    return (mTime2 > mTime ? mTime2 : mTime);
  }
  else
  {
    return mTime;
  }
}

//------------------------------------------------------------------------------
// Specialized implicit function extraction filter to handle unstructured
// grids with 3D linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtk3DLinearGridCrinkleExtractor::ProcessPiece(
  vtkUnstructuredGrid* input, vtkImplicitFunction* f, vtkUnstructuredGrid* grid)
{
  if (input == nullptr || f == nullptr || grid == nullptr)
  {
    // Not really an error
    return 1;
  }

  // Make sure there is input data to process
  vtkPoints* inPts = input->GetPoints();
  vtkIdType numPts = 0;
  if (inPts)
  {
    numPts = inPts->GetNumberOfPoints();
  }
  vtkCellArray* cells = input->GetCells();
  vtkIdType numCells = 0;
  if (cells)
  {
    numCells = cells->GetNumberOfCells();
  }
  if (numPts <= 0 || numCells <= 0)
  {
    vtkLog(TRACE, "Empty input");
    return 0;
  }

  // Check the input point type. Only real types are supported.
  int inPtsType = inPts->GetDataType();
  if ((inPtsType != VTK_FLOAT && inPtsType != VTK_DOUBLE))
  {
    vtkLog(ERROR, "Input point type not supported");
    return 0;
  }

  // Output cells go here.
  vtkNew<vtkCellArray> newCells;

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  CellIter* cellIter = new CellIter(numCells, input->GetCellTypes(), cells);

  // Classify the cell points based on the specified implicit function.
  vtkNew<vtkAOSDataArrayTemplate<unsigned char>> inoutArray;
  inoutArray->SetNumberOfValues(numPts);
  if (auto plane = vtkPlane::SafeDownCast(f))
  {
    ClassifyPointsWorker<vtkPlane> worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          inPts->GetData(), worker, plane, inoutArray, this))
    {
      worker(inPts->GetData(), plane, inoutArray, this);
    }
  }
  else
  { // general implicit function fast path
    ClassifyPointsWorker<vtkImplicitFunction> worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          inPts->GetData(), worker, f, inoutArray, this))
    {
      worker(inPts->GetData(), f, inoutArray, this);
    }
  }
  unsigned char* inout = inoutArray->GetPointer(0);

  // Depending on whether we are going to eliminate unused points, use
  // different extraction techniques. There is a large performance
  // difference if points are compacted.
  vtkIdType outNumCells = 0;
  vtkIdType* cellMap = nullptr;
  vtkIdType outNumPts = 0;
  vtkIdType* ptMap = nullptr;
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  if (!this->RemoveUnusedPoints)
  {
    ExtractCells extract(numPts, cellIter, inout, grid, newCells, this);
    EXECUTE_REDUCED_SMPFOR(
      this->SequentialProcessing, numCells, extract, this->NumberOfThreadsUsed);

    outNumCells = extract.OutputNumCells;
    cellMap = extract.CellMap;

    grid->SetPoints(inPts);
    if (this->CopyPointData)
    {
      vtkPointData* outPD = grid->GetPointData();
      outPD->PassData(inPD);
    }
  }

  else
  {
    ExtractPointsAndCells extract(numPts, cellIter, inout, grid, newCells, this);
    EXECUTE_REDUCED_SMPFOR(
      this->SequentialProcessing, numCells, extract, this->NumberOfThreadsUsed);

    outNumPts = extract.OutputNumPts;
    ptMap = extract.PointMap;

    outNumCells = extract.OutputNumCells;
    cellMap = extract.CellMap;
  }

  // Copy cell data if requested
  if (this->CopyCellData)
  {
    vtkCellData* outCD = grid->GetCellData();
    ArrayList arrays;
    outCD->CopyAllocate(inCD, outNumCells);
    arrays.AddArrays(outNumCells, inCD, outCD);
    CopyCellAttributes copyCellData(&arrays, cellMap, this);
    EXECUTE_SMPFOR(this->SequentialProcessing, outNumCells, copyCellData);
    delete[] cellMap;
  }

  if (this->RemoveUnusedPoints)
  {
    // Create the output points if not passing through. Only real types are
    // supported. Use the point map to create them.
    int inType = inPts->GetDataType(), outType;
    vtkNew<vtkPoints> outPts;
    if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
    {
      outType = inType;
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
    {
      outType = VTK_FLOAT;
    }
    else // if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
      outType = VTK_DOUBLE;
    }
    outPts->SetDataType(outType);
    outPts->SetNumberOfPoints(outNumPts);

    // Generate points using the point map
    using Dispatcher = vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::PointArrays,
      vtkArrayDispatch::AOSPointArrays>;
    GeneratePointsWorker worker;
    if (!Dispatcher::Execute(inPts->GetData(), outPts->GetData(), worker, ptMap, this))
    {
      worker(inPts->GetData(), outPts->GetData(), ptMap, this);
    }
    grid->SetPoints(outPts);

    // Use the point map to copy point data if desired
    if (this->CopyPointData)
    {
      vtkPointData* outPD = grid->GetPointData();
      ArrayList arrays;
      outPD->CopyAllocate(inPD, outNumPts);
      arrays.AddArrays(outNumPts, inPD, outPD);
      CopyPointAttributes copyPointData(&arrays, ptMap, this);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, copyPointData);
      delete[] ptMap;
    }
  }

  // Report the results of execution
  vtkLog(TRACE,
    "Extracted: " << grid->GetNumberOfPoints() << " points, " << grid->GetNumberOfCells()
                  << " cells");

  // Clean up
  delete cellIter;

  return 1;
}

//------------------------------------------------------------------------------
// The output dataset type varies depending on the input type.
int vtk3DLinearGridCrinkleExtractor::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkUnstructuredGrid::SafeDownCast(inputDO))
  {
    if (vtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For any composite dataset, we're create a vtkMultiBlockDataSet as output;
    if (vtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  vtkLog(ERROR, "Not sure what type of output to create!");
  return 0;
}

//------------------------------------------------------------------------------
// Specialized extraction filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int vtk3DLinearGridCrinkleExtractor::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* inputGrid =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* outputGrid =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCompositeDataSet* inputCDS =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkMultiBlockDataSet* outputMBDS =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure we have valid input and output of some form
  if ((inputGrid == nullptr || outputGrid == nullptr) &&
    (inputCDS == nullptr || outputMBDS == nullptr))
  {
    return 0;
  }

  // Need an implicit function to do the cutting
  vtkImplicitFunction* f = this->ImplicitFunction;
  if (!f)
  {
    vtkLog(ERROR, "Implicit function not defined");
    return 0;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output vtkUnstructuredGrid.
  if (inputGrid)
  {
    this->ProcessPiece(inputGrid, f, outputGrid);
  }

  // Otherwise it is an input composite data set and each unstructured grid
  // contained in it is processed, producing a vtkGrid that is added to
  // the output multiblock dataset.
  else
  {
    vtkUnstructuredGrid* grid;
    vtkUnstructuredGrid* output;
    outputMBDS->CopyStructure(inputCDS);
    vtkSmartPointer<vtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCDS->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      if (this->GetAbortOutput())
      {
        break;
      }
      auto ds = inIter->GetCurrentDataObject();
      if ((grid = vtkUnstructuredGrid::SafeDownCast(ds)))
      {
        output = vtkUnstructuredGrid::New();
        this->ProcessPiece(grid, f, output);
        outputMBDS->SetDataSet(inIter, output);
        output->Delete();
      }
      else
      {
        vtkLog(TRACE, << "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtk3DLinearGridCrinkleExtractor::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtk3DLinearGridCrinkleExtractor::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//------------------------------------------------------------------------------
bool vtk3DLinearGridCrinkleExtractor::CanFullyProcessDataObject(vtkDataObject* object)
{
  auto ug = vtkUnstructuredGrid::SafeDownCast(object);
  auto cd = vtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    // Get list of cell types in the unstructured grid
    if (vtkUnsignedCharArray* cellTypes = ug->GetDistinctCellTypesArray())
    {
      for (vtkIdType i = 0; i < cellTypes->GetNumberOfValues(); ++i)
      {
        unsigned char cellType = cellTypes->GetValue(i);
        if (cellType != VTK_EMPTY_CELL && cellType != VTK_VOXEL && cellType != VTK_TETRA &&
          cellType != VTK_HEXAHEDRON && cellType != VTK_WEDGE && cellType != VTK_PYRAMID)
        {
          // Unsupported cell type, can't process data
          return false;
        }
      }
    }

    // All cell types are supported, can process data.
    return true;
  }
  else if (cd)
  {
    bool supported = true;
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    iter->SkipEmptyNodesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto leafDS = iter->GetCurrentDataObject();
      if (!CanFullyProcessDataObject(leafDS))
      {
        supported = false;
        break;
      }
    }
    return supported;
  }

  return false; // not a vtkUnstructuredGrid nor a composite dataset
}

//------------------------------------------------------------------------------
int vtk3DLinearGridCrinkleExtractor::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtk3DLinearGridCrinkleExtractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";

  os << indent << "Copy Point Data: " << (this->CopyPointData ? "true\n" : "false\n");
  os << indent << "Copy Cell Data: " << (this->CopyCellData ? "true\n" : "false\n");

  os << indent << "RemoveUnusedPoints: " << (this->RemoveUnusedPoints ? "true\n" : "false\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
VTK_ABI_NAMESPACE_END
