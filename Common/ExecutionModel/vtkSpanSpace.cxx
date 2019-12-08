/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpanSpace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpanSpace.h"

#include "vtkCell.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkUnstructuredGrid.h"

// Methods and functors for processing in parallel
namespace
{ // begin anonymous namespace

// Compute the scalar range a little faster
template <typename T>
struct ComputeRange
{
  struct LocalDataType
  {
    double Min;
    double Max;
  };

  const T* Scalars;
  double Min;
  double Max;
  vtkSMPThreadLocal<LocalDataType> LocalData;

  ComputeRange(T* s)
    : Scalars(s)
    , Min(VTK_FLOAT_MAX)
    , Max(VTK_FLOAT_MIN)
  {
  }

  void Initialize()
  {
    LocalDataType& localData = this->LocalData.Local();
    localData.Min = VTK_FLOAT_MAX;
    localData.Max = VTK_FLOAT_MIN;
  }

  void operator()(vtkIdType idx, vtkIdType endIdx)
  {
    LocalDataType& localData = this->LocalData.Local();
    double& min = localData.Min;
    double& max = localData.Max;
    const T* s = this->Scalars + idx;

    for (; idx < endIdx; ++idx, ++s)
    {
      min = (*s < min ? *s : min);
      max = (*s > max ? *s : max);
    }
  }

  void Reduce()
  {
    typename vtkSMPThreadLocal<LocalDataType>::iterator ldItr;
    typename vtkSMPThreadLocal<LocalDataType>::iterator ldEnd = this->LocalData.end();

    this->Min = VTK_FLOAT_MAX;
    this->Max = VTK_FLOAT_MIN;

    double min, max;
    for (ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      min = (*ldItr).Min;
      max = (*ldItr).Max;
      this->Min = (min < this->Min ? min : this->Min);
      this->Max = (max > this->Max ? max : this->Max);
    }
  }

  static void Execute(vtkIdType num, T* s, double range[2])
  {
    ComputeRange computeRange(s);
    vtkSMPTools::For(0, num, computeRange);
    range[0] = computeRange.Min;
    range[1] = computeRange.Max;
  }
};

//-----------------------------------------------------------------------------
// The following tuple is an interface between VTK class and internal class
struct vtkSpanTuple
{
  vtkIdType CellId; // originating cellId
  vtkIdType Index;  // i-j index into span space (numCells in length)
  // Operator< used to support sorting operation. Note that the sorting
  // occurs over both the index and cell id. This arranges cells in
  // ascending order (within a bin) which often makes a difference
  //(~10-15%) in large data as it reduces cache misses.
  bool operator<(const vtkSpanTuple& tuple) const
  {
    if (Index < tuple.Index)
      return true;
    if (tuple.Index < Index)
      return false;
    if (CellId < tuple.CellId)
      return true;
    return false;
  }
};

} // anonymous

//-----------------------------------------------------------------------------
// This class manages the span space, including methods to create, access, and
// delete it.
struct vtkInternalSpanSpace
{
  // Okay the various ivars
  vtkIdType Dim;             // the number of rows and number of columns
  double SMin, SMax, Range;  // min and max scalar values; range
  vtkSpanTuple* Space;       //(cellId,s) span space tuples
  vtkIdType* CellIds;        // sorted list of cell ids
  vtkIdType* Offsets;        // offset into CellIds for each bucket (Dim*Dim in size)
  vtkIdType NumCells;        // total number of cells in span space
  vtkIdType* CandidateCells; // to support parallel computing
  vtkIdType NumCandidates;

  // Constructor
  vtkInternalSpanSpace(vtkIdType dim, double sMin, double sMax, vtkIdType numCells);

  // Destructore
  ~vtkInternalSpanSpace();

  // Insert cells with scalar range (smin,smax) in span space. These are
  // sorted later into span space.
  void SetSpanPoint(vtkIdType id, double sMin, double sMax)
  {
    vtkIdType i =
      static_cast<vtkIdType>(static_cast<double>(this->Dim) * (sMin - this->SMin) / this->Range);
    vtkIdType j =
      static_cast<vtkIdType>(static_cast<double>(this->Dim) * (sMax - this->SMin) / this->Range);
    i = (i < 0 ? 0 : (i >= this->Dim ? this->Dim - 1 : i));
    j = (j < 0 ? 0 : (j >= this->Dim ? this->Dim - 1 : j));
    this->Space[id].CellId = id;
    this->Space[id].Index = i + j * Dim;
  }

  // Do the hard work of sorting and arranging the span space
  void Build();

  // Given a scalar value, return a rectangle in span space. This
  // rectangle is used subsequently for extracting individual
  // rows. rMin is the lower (i,j) lower-left corner of the rectangle;
  // rMax is the upper-right corner (i,j) position of the
  // rectangle.
  void GetSpanRectangle(double value, vtkIdType rMin[2], vtkIdType rMax[2])
  {
    vtkIdType i =
      static_cast<vtkIdType>(static_cast<double>(this->Dim) * (value - this->SMin) / this->Range);

    // In the case where value is outside of the span tree scalar range, need
    // to return an empty span rectangle.
    if (i < 0 || i >= this->Dim)
    {
      rMin[0] = rMin[1] = rMax[0] = rMax[1] = 0;
    }
    else // return a non-empty span rectangle
    {
      rMin[0] = 0;     // xmin on rectangle left boundary
      rMin[1] = i;     // ymin on rectangle bottom
      rMax[0] = i + 1; // xmax (non-inclusive interval) on right hand boundary
      rMax[1] = Dim;   // ymax (non-inclusive interval) on top boundary of span space
    }
  }

  // Return an array of cellIds along a prescribed row within the span
  // rectangle.  Note that the row should be inside the
  // rectangle. Note that numCells may be zero in which case the
  // pointer returned will not point to valid data.
  vtkIdType* GetCellsInSpan(
    vtkIdType row, vtkIdType rMin[2], vtkIdType rMax[2], vtkIdType& numCells)
  {
    // Find the beginning of some cells on this row.
    vtkIdType startOffset = *(this->Offsets + row * this->Dim + rMin[0]);
    vtkIdType endOffset = *(this->Offsets + row * this->Dim + rMax[0]);
    numCells = endOffset - startOffset;
    return this->CellIds + startOffset;
  }
};

//-----------------------------------------------------------------------------
vtkInternalSpanSpace::vtkInternalSpanSpace(
  vtkIdType dim, double sMin, double sMax, vtkIdType numCells)
{
  this->Dim = dim;
  this->SMin = sMin;
  this->SMax = sMax;
  this->Range = (sMax - sMin);
  this->Offsets = new vtkIdType[dim * dim + 1]; // leave one extra for numCells
  std::fill_n(this->Offsets, dim * dim, 0);
  this->NumCells = numCells;
  this->Space = new vtkSpanTuple[numCells];
  this->CellIds = new vtkIdType[numCells];
  this->CandidateCells = nullptr;
  this->NumCandidates = 0;
}

//-----------------------------------------------------------------------------
vtkInternalSpanSpace::~vtkInternalSpanSpace()
{
  delete[] this->Offsets;
  delete[] this->Space;
  delete[] this->CellIds;
  delete[] this->CandidateCells;
}

//-----------------------------------------------------------------------------
// The heart of the algorithm. The cells are sorted in i-j space into
// a contiguous array. Then the offsets into the array are built.
void vtkInternalSpanSpace::Build()
{
  // The first thing to do is to sort the elements across span
  // space. The shape of the span space is upper diagonal (because
  // smax >= smin) but for simplicity sake (for now) we just use a
  // rectangular discretization (of dimensions Dim*Dim).
  vtkSMPTools::Sort(this->Space, this->Space + this->NumCells);

  // Now that this is done, we create a matrix of offsets into the
  // sorted array. This enables rapid access into the sorted cellIds,
  // including access to span space rows of cells.  Also for
  // convenience we replicate the cell ids. This further supports
  // parallel traversal which is a common use case. If I was smarter I
  // could use the CellIds already contained in the tuple and not have
  // to duplicate this, but then sorting requires a custom class with
  // iterators, etc.

  // First count the number of contributions in each bucket.
  vtkIdType cellId, numElems;
  for (cellId = 0; cellId < this->NumCells; ++cellId)
  {
    this->Offsets[this->Space[cellId].Index]++;
    this->CellIds[cellId] = this->Space[cellId].CellId;
  }

  // Now accumulate offset array
  vtkIdType i, j, jOffset, idx, currentOffset = 0;
  for (j = 0; j < this->Dim; ++j)
  {
    jOffset = j * this->Dim;
    for (i = 0; i < this->Dim; ++i)
    {
      idx = i + jOffset;
      numElems = this->Offsets[idx];
      this->Offsets[idx] = currentOffset;
      currentOffset += numElems;
    }
  }
  this->Offsets[this->Dim * this->Dim] = this->NumCells;

  // We don't need the span space tuple array any more, we have
  // offsets and cell ids computed.
  delete[] this->Space;
  this->Space = nullptr;
}

namespace
{ // begin anonymous namespace

// Generic method to map cells to span space. Uses GetCellPoints() to retrieve
// points defining each cell.
struct MapToSpanSpace
{
  vtkInternalSpanSpace* SpanSpace;
  vtkDataSet* DataSet;
  vtkDataArray* Scalars;
  vtkSMPThreadLocalObject<vtkIdList> CellPts;
  vtkSMPThreadLocalObject<vtkDoubleArray> CellScalars;

  MapToSpanSpace(vtkInternalSpanSpace* ss, vtkDataSet* ds, vtkDataArray* s)
    : SpanSpace(ss)
    , DataSet(ds)
    , Scalars(s)
  {
  }

  void Initialize()
  {
    vtkIdList*& cellPts = this->CellPts.Local();
    cellPts->SetNumberOfIds(12);
    vtkDoubleArray*& cellScalars = this->CellScalars.Local();
    cellScalars->SetNumberOfTuples(12);
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkIdType j, numScalars;
    double *s, sMin, sMax;
    vtkIdList*& cellPts = this->CellPts.Local();
    vtkDoubleArray*& cellScalars = this->CellScalars.Local();

    for (; cellId < endCellId; ++cellId)
    {
      this->DataSet->GetCellPoints(cellId, cellPts);
      numScalars = cellPts->GetNumberOfIds();
      cellScalars->SetNumberOfTuples(numScalars);
      this->Scalars->GetTuples(cellPts, cellScalars);
      s = cellScalars->GetPointer(0);

      sMin = VTK_DOUBLE_MAX;
      sMax = VTK_DOUBLE_MIN;
      for (j = 0; j < numScalars; j++)
      {
        if (s[j] < sMin)
        {
          sMin = s[j];
        }
        if (s[j] > sMax)
        {
          sMax = s[j];
        }
      } // for all cell scalars
      // Compute span space id, and prepare to map
      this->SpanSpace->SetSpanPoint(cellId, sMin, sMax);
    } // for all cells in this thread
  }

  void Reduce() // Needed because of Initialize()
  {
  }

  static void Execute(vtkIdType numCells, vtkInternalSpanSpace* ss, vtkDataSet* ds, vtkDataArray* s)
  {
    MapToSpanSpace map(ss, ds, s);
    vtkSMPTools::For(0, numCells, map);
  }
}; // MapToSpanSpace

// Specialized method to map unstructured grid cells to span space. Uses
// GetCellPoints() to retrieve points defining the cell.
template <typename TS>
struct MapUGridToSpanSpace
{
  vtkInternalSpanSpace* SpanSpace;
  vtkUnstructuredGrid* Grid;
  TS* Scalars;

  MapUGridToSpanSpace(vtkInternalSpanSpace* ss, vtkUnstructuredGrid* ds, TS* s)
    : SpanSpace(ss)
    , Grid(ds)
    , Scalars(s)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkUnstructuredGrid* grid = this->Grid;
    TS* scalars = this->Scalars;
    vtkIdType i, npts;
    const vtkIdType* pts;
    double s, sMin, sMax;

    for (; cellId < endCellId; ++cellId)
    {
      sMin = VTK_DOUBLE_MAX;
      sMax = VTK_DOUBLE_MIN;
      // A faster version of GetCellPoints()
      grid->GetCellPoints(cellId, npts, pts);
      for (i = 0; i < npts; i++)
      {
        s = static_cast<double>(scalars[pts[i]]);
        sMin = (s < sMin ? s : sMin);
        sMax = (s > sMax ? s : sMax);
      } // for all cell scalars
      // Compute span space id, and prepare to map
      this->SpanSpace->SetSpanPoint(cellId, sMin, sMax);
    } // for all cells in this thread
  }

  static void Execute(vtkIdType numCells, vtkInternalSpanSpace* ss, vtkUnstructuredGrid* ds, TS* s)
  {
    MapUGridToSpanSpace map(ss, ds, s);
    vtkSMPTools::For(0, numCells, map);
  }
}; // MapUGridToSpanSpace

} // anonymous namespace

//---The VTK Classes proper------------------------------------------------------

vtkStandardNewMacro(vtkSpanSpace);

//-----------------------------------------------------------------------------
// Instantiate empty span space object.
vtkSpanSpace::vtkSpanSpace()
{
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->ComputeScalarRange = true;
  this->Resolution = 100;
  this->ComputeResolution = true;
  this->NumberOfCellsPerBucket = 5;
  this->SpanSpace = nullptr;
  this->RMin[0] = this->RMin[1] = 0;
  this->RMax[0] = this->RMax[1] = 0;
  this->BatchSize = 100;
}

//-----------------------------------------------------------------------------
vtkSpanSpace::~vtkSpanSpace()
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
// Shallow copy enough information for a clone to produce the same result on
// the same data.
void vtkSpanSpace::ShallowCopy(vtkScalarTree* stree)
{
  vtkSpanSpace* ss = vtkSpanSpace::SafeDownCast(stree);
  if (ss != nullptr)
  {
    this->SetScalarRange(ss->GetScalarRange());
    this->SetComputeScalarRange(ss->GetComputeScalarRange());
    this->SetResolution(ss->GetResolution());
    this->SetComputeResolution(ss->GetComputeResolution());
    this->SetNumberOfCellsPerBucket(ss->GetNumberOfCellsPerBucket());
  }
  // Now do superclass
  this->Superclass::ShallowCopy(stree);
}

//-----------------------------------------------------------------------------
// Frees memory and resets object as appropriate.
void vtkSpanSpace::Initialize()
{
  if (this->SpanSpace)
  {
    delete this->SpanSpace;
    this->SpanSpace = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Construct the scalar tree / span space from the dataset
// provided. Checks build times and modified time from input and
// reconstructs the tree if necessary.
void vtkSpanSpace::BuildTree()
{
  vtkIdType numCells;

  // Check input...see whether we have to rebuild
  //
  if (!this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1)
  {
    vtkErrorMacro(<< "No data to build tree with");
    return;
  }

  if (this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }

  vtkDebugMacro(<< "Building span space...");

  // If no scalars set then try and grab them from dataset
  if (!this->Scalars)
  {
    this->SetScalars(this->DataSet->GetPointData()->GetScalars());
  }
  if (!this->Scalars)
  {
    vtkErrorMacro(<< "No scalar data to build trees with");
    return;
  }

  // We need a scalar range for the scalars. Do this in parallel for a small
  // boost in performance.
  double range[2];
  void* scalars = this->Scalars->GetVoidPointer(0);

  if (this->ComputeScalarRange)
  {
    switch (this->Scalars->GetDataType())
    {
      vtkTemplateMacro(
        ComputeRange<VTK_TT>::Execute(this->Scalars->GetNumberOfTuples(), (VTK_TT*)scalars, range));
    }
    this->ScalarRange[0] = range[0];
    this->ScalarRange[1] = range[1];
  }
  else
  {
    range[0] = this->ScalarRange[0];
    range[1] = this->ScalarRange[1];
  }

  double R = range[1] - range[0];
  if (R <= 0.0)
  {
    vtkErrorMacro(<< "Bad scalar range");
    return;
  }

  // Prepare to process scalars
  this->Initialize(); // clears out old span space arrays

  // The first pass loops over all cells, mapping them into span space
  // (i.e., an integer id into a gridded span space). Later this id will
  // be used to sort the cells across the span space, so that cells
  // can be processed in order by different threads.
  if (this->ComputeResolution)
  {
    this->Resolution = static_cast<vtkIdType>(
      sqrt(static_cast<double>(numCells) / static_cast<double>(this->NumberOfCellsPerBucket)));
    this->Resolution =
      (this->Resolution < 100 ? 100 : (this->Resolution > 10000 ? 10000 : this->Resolution));
  }
  this->SpanSpace = new vtkInternalSpanSpace(this->Resolution, range[0], range[1], numCells);

  // Acclerated span space construction (for unstructured grids).  Templated
  // over scalar type; direct access to vtkUnstructuredGrid innards.
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(this->DataSet);
  if (ugrid != nullptr)
  {
    switch (this->Scalars->GetDataType())
    {
      vtkTemplateMacro(
        MapUGridToSpanSpace<VTK_TT>::Execute(numCells, this->SpanSpace, ugrid, (VTK_TT*)scalars));
    }
  }

  // Generic, threaded processing of cells to produce span space.
  else
  {
    MapToSpanSpace::Execute(numCells, this->SpanSpace, this->DataSet, this->Scalars);
  }

  // Now sort and build span space
  this->SpanSpace->Build();

  // Update our build time
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
// Begin to traverse the cells based on a scalar value. Returned cells
// will have scalar values that span the scalar value specified.
void vtkSpanSpace::InitTraversal(double scalarValue)
{
  this->BuildTree();
  this->ScalarValue = scalarValue;

  // Find the rectangle in span space that spans the isovalue
  this->SpanSpace->GetSpanRectangle(scalarValue, this->RMin, this->RMax);

  // Initiate the serial looping over all span rows
  this->CurrentRow = this->RMin[1];
  this->CurrentSpan = this->SpanSpace->GetCellsInSpan(
    this->CurrentRow, this->RMin, this->RMax, this->CurrentNumCells);
  this->CurrentIdx = 0; // beginning of current span row
}

//-----------------------------------------------------------------------------
// Return the next cell that may contain scalar value specified to
// initialize traversal. The value nullptr is returned if the list is
// exhausted. Make sure that InitTraversal() has been invoked first or
// you'll get erratic behavior. This is serial traversal.
vtkCell* vtkSpanSpace::GetNextCell(
  vtkIdType& cellId, vtkIdList*& cellPts, vtkDataArray* cellScalars)
{
  // Where are we in the current span space row? If at the end, need to get the
  // next row (or return if the last row)
  while (this->CurrentIdx >= this->CurrentNumCells)
  {
    this->CurrentRow++;
    if (this->CurrentRow >= this->RMax[1])
    {
      return nullptr;
    }
    else
    {
      this->CurrentSpan = this->SpanSpace->GetCellsInSpan(
        this->CurrentRow, this->RMin, this->RMax, this->CurrentNumCells);
      this->CurrentIdx = 0; // beginning of row
    }
  }

  // If here then get the next cell
  vtkIdType numScalars;
  vtkCell* cell;
  cellId = this->CurrentSpan[this->CurrentIdx++];
  cell = this->DataSet->GetCell(cellId);
  cellPts = cell->GetPointIds();
  numScalars = cellPts->GetNumberOfIds();
  cellScalars->SetNumberOfTuples(numScalars);
  this->Scalars->GetTuples(cellPts, cellScalars);

  return cell;
}

//-----------------------------------------------------------------------------
// Note the cell ids are copied into memory (CandidateCells) from
// which batches are created. This is done for load balancing
// purposes. The span space can often aggregate many cells in just a
// few bins; meaning that batches cannot just be span rows if the work
// is to shared across many threads.
vtkIdType vtkSpanSpace::GetNumberOfCellBatches(double scalarValue)
{
  // Make sure tree is built, modified time will prevent reexecution.
  this->BuildTree();
  this->ScalarValue = scalarValue;

  // Find the rectangle in span space that spans the isovalue
  vtkInternalSpanSpace* sp = this->SpanSpace;
  ;
  sp->GetSpanRectangle(scalarValue, this->RMin, this->RMax);

  // Loop over each span row to count total memory allocation required.
  vtkIdType numCandidates = 0;
  vtkIdType row, *span, idx, numCells;
  for (row = this->RMin[1]; row < this->RMax[1]; ++row)
  {
    sp->GetCellsInSpan(row, this->RMin, this->RMax, numCells);
    numCandidates += numCells;
  } // for all rows in span rectangle

  // Allocate list of candidate cells. Cache memory to avoid
  // reallocation if possible.
  if (sp->CandidateCells != nullptr && numCandidates > sp->NumCandidates)
  {
    delete[] sp->CandidateCells;
    sp->CandidateCells = nullptr;
  }
  sp->NumCandidates = numCandidates;
  if (numCandidates > 0 && sp->CandidateCells == nullptr)
  {
    sp->CandidateCells = new vtkIdType[sp->NumCandidates];
  }

  // Now copy cells into the allocated memory. This could be done in
  // parallel (a parallel write - TODO) but probably wouldn't provide
  // much of a boost.
  numCandidates = 0;
  for (row = this->RMin[1]; row < this->RMax[1]; ++row)
  {
    span = sp->GetCellsInSpan(row, this->RMin, this->RMax, numCells);
    for (idx = 0; idx < numCells; ++idx)
    {
      sp->CandidateCells[numCandidates++] = span[idx];
    }
  } // for all rows in span rectangle

  // Watch for boundary conditions. Return BatchSize cells to a batch.
  if (sp->NumCandidates < 1)
  {
    return 0;
  }
  else
  {
    return (((sp->NumCandidates - 1) / this->BatchSize) + 1);
  }
}

//-----------------------------------------------------------------------------
// Call after GetNumberOfCellBatches(isoValue)
const vtkIdType* vtkSpanSpace::GetCellBatch(vtkIdType batchNum, vtkIdType& numCells)
{
  // Make sure that everything is hunky dory
  vtkInternalSpanSpace* sp = this->SpanSpace;
  ;
  vtkIdType pos = batchNum * this->BatchSize;
  if (sp->NumCells < 1 || !sp->CandidateCells || pos >= sp->NumCandidates)
  {
    numCells = 0;
    return nullptr;
  }

  // Return a batch, or if near the end of the candidate list,
  // the remainder batch.
  if ((sp->NumCandidates - pos) >= this->BatchSize)
  {
    numCells = this->BatchSize;
  }
  else
  {
    numCells = sp->NumCandidates % this->BatchSize;
  }

  return sp->CandidateCells + pos;
}

//-----------------------------------------------------------------------------
void vtkSpanSpace::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << "," << this->ScalarRange[1] << ")\n";
  os << indent << "Compute Scalar Range: " << (this->ComputeScalarRange ? "On\n" : "Off\n");
  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Compute Resolution: " << (this->ComputeResolution ? "On\n" : "Off\n");
  os << indent << "Number of Cells Per Bucket: " << this->NumberOfCellsPerBucket << "\n";
}
