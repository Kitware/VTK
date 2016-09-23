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
#include "vtkGenericCell.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

#include <algorithm> //std::sort

//-----------------------------------------------------------------------------
// The following tuple is an interface between VTK class and internal class
struct vtkSpanTuple
{
  vtkIdType CellId; //originating cellId
  vtkIdType Index; //i-j index into span space (numCells in length)
  //Operator< used to support sorting operation.
  bool operator< (const vtkSpanTuple& tuple) const
    {return Index < tuple.Index;}
};


//-----------------------------------------------------------------------------
// This class manages the span space, including methods to create, access, and
// delete it.
class vtkInternalSpanSpace
{
public:
  // Okay the various ivars
  vtkIdType Dim; //the number of rows and number of columns
  double SMin, SMax, Range; //min and max scalar values; range
  vtkSpanTuple *Space; //(cellId,s) span space tuples
  vtkIdType *CellIds; //sorted list of cell ids
  vtkIdType *Offsets; //offset into CellIds for each bucket (Dim*Dim in size)
  vtkIdType NumCells; //total number of cells in span space
  vtkIdType *CandidateCells; //to support parallel computing
  vtkIdType  NumCandidates;

  // Constructor
  vtkInternalSpanSpace(vtkIdType dim, double sMin, double sMax, vtkIdType numCells);

  // Destructore
  ~vtkInternalSpanSpace();

  // Insert cells with scalar range (smin,smax) in span space
  void SetSpanPoint(vtkIdType id, double sMin, double sMax)
  {
    vtkIdType i = static_cast<vtkIdType>(
      static_cast<double>(this->Dim) * (sMin - this->SMin) / this->Range);
    vtkIdType j = static_cast<vtkIdType>(
      static_cast<double>(this->Dim) * (sMax - this->SMin) / this->Range);
    i = ( i < 0 ? 0 : (i >= this->Dim ? this->Dim-1 : i));
    j = ( j < 0 ? 0 : (j >= this->Dim ? this->Dim-1 : j));
    this->Space[id].CellId = id;
    this->Space[id].Index = i + j*Dim;
  }

  // Do the hard work of sorting and arranging the span space
  void Build();

  // Given a scalar value, return a rectangle in span space. This
  // rectangle is used subsequently for extracing individual
  // rows. rMin is the lower (i,j) lower-left corner of the rectangle;
  // rMax is the upper-right corner (i,j) position of the
  // rectangle.
  void GetSpanRectangle(double value, vtkIdType rMin[2], vtkIdType rMax[2])
  {
    vtkIdType i = static_cast<vtkIdType>(
      static_cast<double>(this->Dim) * (value - this->SMin) / this->Range);

    rMin[0] = 0; //xmin on rectangle left boundary
    rMin[1] = i; //ymin on rectangle bottom
    rMax[0] = i+1; //xmax (non-inclusive interval) on right hand boundary
    rMax[1] = Dim; //ymax (non-inclusive interval) on top boundary of span space
  }

  // Return an array of cellIds along a prescribed row within the span
  // rectangle.  Note that the row should be inside the
  // rectangle. Note that numCells may be zero in which case the
  // pointer returned will not point to valid data.
  vtkIdType *GetCellsInSpan(vtkIdType row, vtkIdType rMin[2], vtkIdType rMax[2],
                            vtkIdType& numCells)
  {
    // Find the beginning of some cells on this row.
    vtkIdType startOffset = *(this->Offsets + row*this->Dim + rMin[0]);
    vtkIdType endOffset = *(this->Offsets + row*this->Dim + rMax[0]);
    numCells = endOffset - startOffset;
    return this->CellIds + startOffset;
  }

  class MapToSpanSpace
  {
  public:
    vtkInternalSpanSpace *SpanSpace;
    vtkDataSet *DataSet;
    vtkDataArray *Scalars;
    vtkSMPThreadLocalObject<vtkIdList> CellPts;
    vtkSMPThreadLocalObject<vtkDoubleArray> CellScalars;

    MapToSpanSpace(vtkInternalSpanSpace *ss, vtkDataSet *ds, vtkDataArray *s) :
      SpanSpace(ss), DataSet(ds), Scalars(s)
    {
    }

    void Initialize()
    {
      vtkIdList*& cellPts = this->CellPts.Local();
      cellPts->SetNumberOfIds(12);
      vtkDoubleArray*& cellScalars = this->CellScalars.Local();
      cellScalars->SetNumberOfTuples(12);
    }

    void operator() (vtkIdType cellId, vtkIdType endCellId)
    {
      vtkIdType j, numScalars;
      double *s, sMin, sMax;
      vtkIdList*& cellPts = this->CellPts.Local();
      vtkDoubleArray*& cellScalars = this->CellScalars.Local();

      for ( ; cellId < endCellId; ++cellId )
      {
        this->DataSet->GetCellPoints(cellId,cellPts);
        numScalars = cellPts->GetNumberOfIds();
        cellScalars->SetNumberOfTuples(numScalars);
        this->Scalars->GetTuples(cellPts, cellScalars);
        s = cellScalars->GetPointer(0);

        sMin = VTK_DOUBLE_MAX;
        sMax = VTK_DOUBLE_MIN;
        for ( j=0; j < numScalars; j++ )
        {
          if ( s[j] < sMin )
          {
            sMin = s[j];
          }
          if ( s[j] > sMax )
          {
            sMax = s[j];
          }
        }//for all cell scalars
        // Compute span space id, and prepare to map
        this->SpanSpace->SetSpanPoint(cellId, sMin, sMax);
      }//for all cells in this thread
    }

    void Reduce()
    {
    }
  };
};

//-----------------------------------------------------------------------------
vtkInternalSpanSpace::
vtkInternalSpanSpace(vtkIdType dim, double sMin, double sMax, vtkIdType numCells)
{
  this->Dim = (dim > 0 ? dim : 256);
  this->SMin = sMin;
  this->SMax = sMax;
  this->Range = (sMax - sMin);
  this->Offsets = new vtkIdType [dim*dim+1]; //leave one extra for numCells
  std::fill_n(this->Offsets, dim*dim, 0);
  this->NumCells = numCells;
  this->Space = new vtkSpanTuple [numCells];
  this->CellIds = new vtkIdType [numCells];
  this->CandidateCells = NULL;
  this->NumCandidates = 0;
}

//-----------------------------------------------------------------------------
vtkInternalSpanSpace::
~vtkInternalSpanSpace()
{
  delete [] this->Offsets;
  delete [] this->Space;
  delete [] this->CellIds;
  delete [] this->CandidateCells;
}

//-----------------------------------------------------------------------------
// The heart of the algorithm. The cells are sorted in i-j space into
// a contiguous array. Then the offsets into the array are built.
void vtkInternalSpanSpace::
Build()
{
  // The first thing to do is to sort the elements across span
  // space. The shape of the span space is upper diagonal (because
  // smax >= smin) but for simplicity sake (for now) we just use a
  // rectangular discretization (of dimensions Dim*Dim).
  vtkSMPTools::Sort(this->Space,this->Space+this->NumCells);

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
  for ( cellId=0; cellId < this->NumCells; ++cellId )
  {
    this->Offsets[this->Space[cellId].Index]++;
    this->CellIds[cellId] = this->Space[cellId].CellId;
  }

  // Now accumulate offset array
  vtkIdType i, j, jOffset, idx, currentOffset = 0;
  for (j=0; j < this->Dim; ++j)
  {
    jOffset = j * this->Dim;
    for (i=0; i < this->Dim; ++i)
    {
      idx = i + jOffset;
      numElems = this->Offsets[idx];
      this->Offsets[idx] = currentOffset;
      currentOffset += numElems;
    }
  }
  this->Offsets[this->Dim*this->Dim] = this->NumCells;

  // We don't need the span space tuple array any more, we have
  // offsets and cell ids computed.
  delete [] this->Space;
  this->Space = NULL;

  // The candidate cell list can be allocated
  if ( this->CandidateCells )
  {
    delete [] this->CandidateCells;
    this->CandidateCells = NULL;
  }
  this->CandidateCells = new vtkIdType [this->NumCells];
}


//---The VTK Class proper------------------------------------------------------
vtkStandardNewMacro(vtkSpanSpace);

//-----------------------------------------------------------------------------
// Instantiate empty span space object.
vtkSpanSpace::vtkSpanSpace()
{
  this->SpanSpace = NULL;
  this->RMin[0] = this->RMin[1] = 0;
  this->RMax[0] = this->RMax[1] = 0;
  this->BatchSize = 10;
  this->Resolution = 100;
}

//-----------------------------------------------------------------------------
vtkSpanSpace::~vtkSpanSpace()
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
// Frees memory and resets object as appropriate.
void vtkSpanSpace::Initialize()
{
  if (this->SpanSpace)
  {
    delete this->SpanSpace;
    this->SpanSpace = NULL;
  }
}

//-----------------------------------------------------------------------------
// Construct the scalar tree / span space from the dataset
// provided. Checks build times and modified time from input and
// reconstructs the tree if necessaery.
void vtkSpanSpace::BuildTree()
{
  vtkIdType numCells;

  // Check input...see whether we have to rebuild
  //
  if ( !this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1 )
  {
    vtkErrorMacro( << "No data to build tree with");
    return;
  }

  if ( this->BuildTime > this->MTime
       && this->BuildTime > this->DataSet->GetMTime() )
  {
    return;
  }

  vtkDebugMacro( << "Building span space..." );

  // If no scalars set then try and grab them from dataset
  if ( ! this->Scalars )
  {
    this->SetScalars(this->DataSet->GetPointData()->GetScalars());
  }
  if ( ! this->Scalars )
  {
    vtkErrorMacro( << "No scalar data to build trees with");
    return;
  }

  // We need a range for the scalars
  double range[2];
  this->Scalars->GetRange(range);
  double  R = range[1] - range[0];
  if ( R <= 0.0 )
  {
    vtkErrorMacro( << "Bad scalar range");
    return;
  }

  // Prepare to process scalars
  this->Initialize(); //clears out old span space arrays

  // The first pass loops over all cells, mapping them into span space
  // (i.e., an integer id into a gridded span space). Later this id will
  // be used to sort the cells across the span space, so that cells
  // can be processed in order by different threads.
  this->SpanSpace = new
    vtkInternalSpanSpace(this->Resolution,range[0],range[1],numCells);

  // Threaded processing of cells to produce span space
  vtkInternalSpanSpace::
    MapToSpanSpace map(this->SpanSpace,this->DataSet,this->Scalars);
  vtkSMPTools::For(0,numCells,map);

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
  this->CurrentSpan = this->SpanSpace->
    GetCellsInSpan(this->CurrentRow, this->RMin, this->RMax,
                   this->CurrentNumCells);
  this->CurrentIdx = 0; //beginning of current span row
}

//-----------------------------------------------------------------------------
// Return the next cell that may contain scalar value specified to
// initialize traversal. The value NULL is returned if the list is
// exhausted. Make sure that InitTraversal() has been invoked first or
// you'll get erratic behavior. This is serial traversal.
vtkCell *vtkSpanSpace::GetNextCell(vtkIdType& cellId, vtkIdList* &cellPts,
                                   vtkDataArray *cellScalars)
{
  // Where are we in the current span space row? If at the end, need to get the
  // next row (or return if the last row)
  while ( this->CurrentIdx >= this->CurrentNumCells )
  {
    this->CurrentRow++;
    if (this->CurrentRow >= this->RMax[1])
    {
      return NULL;
    }
    else
    {
      this->CurrentSpan = this->SpanSpace->
        GetCellsInSpan(this->CurrentRow, this->RMin, this->RMax,
                       this->CurrentNumCells);
      this->CurrentIdx = 0; //beginning of row
    }
  }

  // If here then get the next cell
  vtkIdType numScalars;
  vtkCell *cell;
  cellId = this->CurrentSpan[this->CurrentIdx++];
  cell = this->DataSet->GetCell(cellId);
  cellPts = cell->GetPointIds();
  numScalars = cellPts->GetNumberOfIds();
  cellScalars->SetNumberOfTuples(numScalars);
  this->Scalars->GetTuples(cellPts, cellScalars);

  return cell;
}

//-----------------------------------------------------------------------------
// Return the number of cell batches. Here we are going to consider a batch the
// number of span space buckets included in the current span rectange. Note that
// InitTraversal() must have been called, which populates the span rectangle.
vtkIdType vtkSpanSpace::GetNumberOfCellBatches()
{
  // Basicall just perform a serial traversal and populate candidate list
  this->SpanSpace->NumCandidates = 0;

  // loop over all rows in span rectangle
  vtkIdType row, *span, idx, numCells;
  for (row=this->RMin[1]; row < this->RMax[1]; ++row)
  {
    span = this->SpanSpace->
      GetCellsInSpan(row, this->RMin, this->RMax, numCells);
    for (idx=0; idx < numCells; ++idx)
    {
      this->SpanSpace->
        CandidateCells[this->SpanSpace->NumCandidates++] = span[idx];
    }
  }//for all rows in span rectangle

  // Watch for boundary conditions. Return 10 cells to a batch.
  if ( this->SpanSpace->NumCandidates < 1 )
  {
    return 0;
  }
  else
  {
    return ( ((this->SpanSpace->NumCandidates-1)/this->BatchSize) + 1);
  }
}

//-----------------------------------------------------------------------------
// Return the next list of cells to process.
const vtkIdType* vtkSpanSpace::
GetCellBatch(vtkIdType batchNum, vtkIdType& numCells)
{
  // Make sure that everything is hunky dory
  vtkIdType pos = batchNum * this->BatchSize;
  if ( this->SpanSpace->NumCells < 1 || ! this->SpanSpace->CandidateCells ||
       pos > this->SpanSpace->NumCandidates )
  {
    numCells = 0;
    return NULL;
  }

  if ( (this->SpanSpace->NumCandidates - pos) >= this->BatchSize )
  {
    numCells = this->BatchSize;
  }
  else
  {
    numCells = this->SpanSpace->NumCandidates % this->BatchSize;
  }

  return this->SpanSpace->CandidateCells + pos;
}

//-----------------------------------------------------------------------------
void vtkSpanSpace::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n" ;
}
