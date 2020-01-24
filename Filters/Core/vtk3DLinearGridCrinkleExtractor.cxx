/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DLinearGridCrinkleExtractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk3DLinearGridCrinkleExtractor.h"

#include "vtk3DLinearGridInternal.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
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
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <atomic>
#include <vector>

vtkStandardNewMacro(vtk3DLinearGridCrinkleExtractor);
vtkCxxSetObjectMacro(vtk3DLinearGridCrinkleExtractor, ImplicitFunction, vtkImplicitFunction);

//-----------------------------------------------------------------------------
// Macros immediately below are just used to make code easier to
// read. Invokes functor _op _num times depending on serial (_seq==1) or
// parallel processing mode. The _REDUCE_ version is used to called functors
// with a Reduce() method).
#define EXECUTE_SMPFOR(_seq, _num, _op)                                                            \
  if (!_seq)                                                                                       \
  {                                                                                                \
    vtkSMPTools::For(0, _num, _op);                                                                \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    _op(0, _num);                                                                                  \
  }

#define EXECUTE_REDUCED_SMPFOR(_seq, _num, _op, _nt)                                               \
  if (!_seq)                                                                                       \
  {                                                                                                \
    vtkSMPTools::For(0, _num, _op);                                                                \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    _op.Initialize();                                                                              \
    _op(0, _num);                                                                                  \
    _op.Reduce();                                                                                  \
  }                                                                                                \
  _nt = _op.NumThreadsUsed;

namespace
{ // anonymous

//========================= Quick implicit function cell selection =============

// Compute an array that classifies each point with respect to the current
// implicit function (i.e. above the function(=2), below the function(=1), on
// the function(=0)).  InOutArray is allocated here and should be deleted by
// the invoking code. InOutArray is an unsigned char array to simplify bit
// fiddling later on (i.e., Intersects() method). A fast path is available
// for vtkPlane implicit functions.
//
// The reason we compute this unsigned char array as compared to an array of
// function values is to reduce the amount of memory used, and writing to
// memory, since these are significant costs for large data.

// Templated for explicit point representations of real type
template <typename TP>
struct PlaneClassifyPoints;
template <typename TP>
struct FunctionClassifyPoints;

// General classification capability
struct Classify
{
  unsigned char* InOutArray;

  Classify(vtkPoints* pts) { this->InOutArray = new unsigned char[pts->GetNumberOfPoints()]; }

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
};

// Faster path for vtkPlane
template <typename TP>
struct PlaneClassifyPoints : public Classify
{
  TP* Points;
  double Origin[3];
  double Normal[3];

  PlaneClassifyPoints(vtkPoints* pts, vtkPlane* plane)
    : Classify(pts)
  {
    this->Points = static_cast<TP*>(pts->GetVoidPointer(0));
    plane->GetOrigin(this->Origin);
    plane->GetNormal(this->Normal);
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double p[3], zero = double(0), eval;
    double *n = this->Normal, *o = this->Origin;
    TP* pts = this->Points + 3 * ptId;
    unsigned char* ioa = this->InOutArray + ptId;
    for (; ptId < endPtId; ++ptId)
    {
      // Access each point
      p[0] = static_cast<double>(*pts);
      ++pts;
      p[1] = static_cast<double>(*pts);
      ++pts;
      p[2] = static_cast<double>(*pts);
      ++pts;

      // Evaluate position of the point with the plane. Invoke inline,
      // non-virtual version of evaluate method.
      eval = vtkPlane::Evaluate(n, o, p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *ioa++ = (eval > zero ? 2 : (eval < zero ? 1 : 0));
    }
  }
};

// General path for vtkImplicitFunction
template <typename TP>
struct FunctionClassifyPoints : public Classify
{
  TP* Points;
  vtkImplicitFunction* Function;

  FunctionClassifyPoints(vtkPoints* pts, vtkImplicitFunction* f)
    : Classify(pts)
    , Function(f)
  {
    this->Points = static_cast<TP*>(pts->GetVoidPointer(0));
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    double p[3], zero = double(0), eval;
    TP* pts = this->Points + 3 * ptId;
    unsigned char* ioa = this->InOutArray + ptId;
    for (; ptId < endPtId; ++ptId)
    {
      // Access each point
      p[0] = static_cast<double>(*pts);
      ++pts;
      p[1] = static_cast<double>(*pts);
      ++pts;
      p[2] = static_cast<double>(*pts);
      ++pts;

      // Evaluate position of the point wrt the implicit function. This
      // call better be thread safe.
      eval = this->Function->FunctionValue(p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *ioa++ = (eval > zero ? 2 : (eval < zero ? 1 : 0));
    }
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

  ExtractCellsBase(vtkIdType inNumPts, CellIter* c, unsigned char* inout, vtkUnstructuredGrid* grid,
    vtkCellArray* cells, bool copyPtData, bool copyCellData)
    : InOut(inout)
    , Iter(c)
    , InputNumPts(inNumPts)
    , OutputNumPts(0)
    , OutputNumCells(0)
    , TotalSize(0)
    , Grid(grid)
    , Cells(cells)
    , CopyPointData(copyPtData)
    , CopyCellData(copyCellData)
    , PointMap(nullptr)
    , CellMap(nullptr)
    , NumThreadsUsed(0)
  {
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
    vtkCellArray* cells, bool copyPtData, bool copyCellData)
    : ExtractCellsBase(inNumPts, c, inout, grid, cells, copyPtData, copyCellData)
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

    for (; cellId < endCellId; ++cellId)
    {
      // Does the implicit function cut this cell?
      npts = cellIter->NumVerts;
      if (Classify::Intersects(inout, npts, c))
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
    vtkUnstructuredGrid* grid, vtkCellArray* cells, bool copyPtData, bool copyCellData)
    : ExtractCellsBase(inNumPts, c, inout, grid, cells, copyPtData, copyCellData)
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

    for (; cellId < endCellId; ++cellId)
    {
      // Does the implicit function cut this cell?
      npts = cellIter->NumVerts;
      if (Classify::Intersects(inout, npts, c))
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

  CopyCellAttributes(ArrayList* arrays, const vtkIdType* cellMap)
    : Arrays(arrays)
    , CellMap(cellMap)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    for (; cellId < endCellId; ++cellId)
    {
      this->Arrays->Copy(this->CellMap[cellId], cellId);
    }
  }
};

// Generate point coordinates
template <typename TPIn, typename TPOut>
struct GeneratePoints
{
  const TPIn* InPts;
  const vtkIdType* PointMap;
  TPOut* OutPts;

  GeneratePoints(TPIn* inPts, vtkIdType* ptMap, TPOut* outPts)
    : InPts(inPts)
    , PointMap(ptMap)
    , OutPts(outPts)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const TPIn* p = this->InPts + 3 * ptId;
    const vtkIdType* ptMap = this->PointMap;
    TPOut *outPts = this->OutPts, *x;

    for (; ptId < endPtId; ++ptId, p += 3)
    {
      if (ptMap[ptId] >= 0)
      {
        x = outPts + 3 * ptMap[ptId];
        *x++ = static_cast<TPOut>(p[0]);
        *x++ = static_cast<TPOut>(p[1]);
        *x = static_cast<TPOut>(p[2]);
      }
    }
  }
};

// Copy point data from input to output.
struct CopyPointAttributes
{
  ArrayList* Arrays;
  const vtkIdType* PointMap;

  CopyPointAttributes(ArrayList* arrays, const vtkIdType* ptMap)
    : Arrays(arrays)
    , PointMap(ptMap)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    const vtkIdType* ptMap = this->PointMap;
    for (; ptId < endPtId; ++ptId)
    {
      if (ptMap[ptId] >= 0)
      {
        this->Arrays->Copy(ptId, ptMap[ptId]);
      }
    }
  }
};

} // anonymous namespace

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
vtk3DLinearGridCrinkleExtractor::~vtk3DLinearGridCrinkleExtractor()
{
  this->SetImplicitFunction(nullptr);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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
    vtkLog(INFO, "Empty input");
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
  vtkCellArray* newCells = vtkCellArray::New();

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  unsigned char* cellTypes =
    static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  CellIter* cellIter = new CellIter(numCells, cellTypes, cells);

  // Classify the cell points based on the specified implicit function. A
  // fast path is available for planes.
  unsigned char* inout = nullptr;
  int ptsType = inPts->GetDataType();
  if (vtkPlane::SafeDownCast(f) != nullptr)
  { // plane fast path
    if (ptsType == VTK_FLOAT)
    {
      PlaneClassifyPoints<float> classify(inPts, static_cast<vtkPlane*>(f));
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
    else if (ptsType == VTK_DOUBLE)
    {
      PlaneClassifyPoints<double> classify(inPts, static_cast<vtkPlane*>(f));
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
  }
  else
  { // general implicit function fast path
    if (ptsType == VTK_FLOAT)
    {
      FunctionClassifyPoints<float> classify(inPts, f);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
    else if (ptsType == VTK_DOUBLE)
    {
      FunctionClassifyPoints<double> classify(inPts, f);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
  }

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
    ExtractCells extract(
      numPts, cellIter, inout, grid, newCells, this->CopyPointData, this->CopyCellData);
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
    ExtractPointsAndCells extract(
      numPts, cellIter, inout, grid, newCells, this->CopyPointData, this->CopyCellData);
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
    CopyCellAttributes copyCellData(&arrays, cellMap);
    EXECUTE_SMPFOR(this->SequentialProcessing, outNumCells, copyCellData);
    delete[] cellMap;
  }

  if (this->RemoveUnusedPoints)
  {
    // Create the output points if not passing through. Only real types are
    // supported. Use the point map to create them.
    int inType = inPts->GetDataType(), outType;
    void *inPtr, *outPtr;
    vtkPoints* outPts = vtkPoints::New();
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
    inPtr = inPts->GetData()->GetVoidPointer(0);
    outPtr = outPts->GetData()->GetVoidPointer(0);
    if (inType == VTK_DOUBLE && outType == VTK_DOUBLE)
    {
      GeneratePoints<double, double> generatePts((double*)inPtr, ptMap, (double*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    else if (inType == VTK_FLOAT && outType == VTK_FLOAT)
    {
      GeneratePoints<float, float> generatePts((float*)inPtr, ptMap, (float*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    else if (inType == VTK_DOUBLE && outType == VTK_FLOAT)
    {
      GeneratePoints<double, float> generatePts((double*)inPtr, ptMap, (float*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    else // if ( inType == VTK_FLOAT && outType == VTK_DOUBLE )
    {
      GeneratePoints<float, double> generatePts((float*)inPtr, ptMap, (double*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    grid->SetPoints(outPts);
    outPts->Delete();

    // Use the point map to copy point data if desired
    if (this->CopyPointData)
    {
      vtkPointData* outPD = grid->GetPointData();
      ArrayList arrays;
      outPD->CopyAllocate(inPD, outNumPts);
      arrays.AddArrays(outNumPts, inPD, outPD);
      CopyPointAttributes copyPointData(&arrays, ptMap);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, copyPointData);
      delete[] ptMap;
    }
  }

  // Report the results of execution
  vtkLog(INFO,
    "Extracted: " << grid->GetNumberOfPoints() << " points, " << grid->GetNumberOfCells()
                  << " cells");

  // Clean up
  if (inout != nullptr)
  {
    delete[] inout;
  }
  delete cellIter;
  newCells->Delete();

  return 1;
}

//----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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
        vtkLog(INFO, << "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtk3DLinearGridCrinkleExtractor::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtk3DLinearGridCrinkleExtractor::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
bool vtk3DLinearGridCrinkleExtractor::CanFullyProcessDataObject(vtkDataObject* object)
{
  auto ug = vtkUnstructuredGrid::SafeDownCast(object);
  auto cd = vtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    // Get list of cell types in the unstructured grid
    vtkNew<vtkCellTypes> cellTypes;
    ug->GetCellTypes(cellTypes);
    for (vtkIdType i = 0; i < cellTypes->GetNumberOfTypes(); ++i)
    {
      unsigned char cellType = cellTypes->GetCellType(i);
      if (cellType != VTK_VOXEL && cellType != VTK_TETRA && cellType != VTK_HEXAHEDRON &&
        cellType != VTK_WEDGE && cellType != VTK_PYRAMID)
      {
        // Unsupported cell type, can't process data
        return false;
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

//-----------------------------------------------------------------------------
int vtk3DLinearGridCrinkleExtractor::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
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
