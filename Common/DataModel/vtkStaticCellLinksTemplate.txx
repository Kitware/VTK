/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCellLinksTemplate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticCellLinksTemplate.h"

#ifndef vtkStaticCellLinksTemplate_txx
#define vtkStaticCellLinksTemplate_txx

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
// Note: this class is a faster, serial version of vtkCellLinks. Future work
// to parallelize this class is possible. This includes:
// = using a parallel prefix sum operation
// = using atomics to update counts (i.e., number of cells using a point)

//----------------------------------------------------------------------------
// Clean up any previously allocated memory
template <typename TIds> void vtkStaticCellLinksTemplate<TIds>::
Initialize()
{
  if ( this->Links )
  {
    delete [] this->Links;
    this->Links = NULL;
  }
  if ( this->Offsets )
  {
    delete [] this->Offsets;
    this->Offsets = NULL;
  }
}

//----------------------------------------------------------------------------
// Build the link list array for any dataset type. Specialized methods are
// used for dataset types that use vtkCellArrays to represent cells.
template <typename TIds> void vtkStaticCellLinksTemplate<TIds>::
BuildLinks(vtkDataSet *ds)
{
  // Use a fast path if polydata or unstructured grid
  if ( ds->GetDataObjectType() == VTK_POLY_DATA )
  {
    return this->BuildLinks(static_cast<vtkPolyData*>(ds));
  }

  else if ( ds->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
  {
    return this->BuildLinks(static_cast<vtkUnstructuredGrid*>(ds));
  }

  // Any other type of dataset. Generally this is not called as datasets have
  // their own, more efficient ways of getting similar information.
  // Make sure that we clear out previous allocation.
  this->NumCells = ds->GetNumberOfCells();
  this->NumPts = ds->GetNumberOfPoints();

  vtkIdType npts, ptId;
  vtkIdType cellId, j;
  vtkIdList *cellPts = vtkIdList::New();

  // Traverse data to determine number of uses of each point. Also count the
  // number of links to allocate.
  this->Offsets = new TIds[this->NumPts+1];
  std::fill_n(this->Offsets, this->NumPts, 0);

  for (this->LinksSize=0, cellId=0; cellId < this->NumCells; cellId++)
  {
    ds->GetCellPoints(cellId,cellPts);
    npts = cellPts->GetNumberOfIds();
    for (j=0; j < npts; j++)
    {
      this->Offsets[cellPts->GetId(j)]++;
      this->LinksSize++;
    }
  }

  // Allocate space for links. Perform prefix sum.
  this->Links = new TIds[this->LinksSize+1];
  this->Links[this->LinksSize] = this->NumPts;

  for ( ptId=0; ptId < this->NumPts; ++ptId )
  {
    npts = this->Offsets[ptId+1];
    this->Offsets[ptId+1] = this->Offsets[ptId] + npts;
  }

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for ( cellId=0; cellId < this->NumCells; ++cellId )
  {
    ds->GetCellPoints(cellId,cellPts);
    npts = cellPts->GetNumberOfIds();
    for (j=0; j<npts; ++j)
    {
      ptId = cellPts->GetId(j);
      this->Offsets[ptId]--;
      this->Links[this->Offsets[ptId]] = cellId;
    }
  }
  this->Offsets[this->NumPts] = this->LinksSize;

  cellPts->Delete();
}

//----------------------------------------------------------------------------
// Build the link list array for unstructured grids
template <typename TIds> void vtkStaticCellLinksTemplate<TIds>::
BuildLinks(vtkUnstructuredGrid *ugrid)
{
  // Basic information about the grid
  this->NumCells = ugrid->GetNumberOfCells();
  this->NumPts = ugrid->GetNumberOfPoints();

  // We're going to get into the guts of the class
  vtkCellArray *cellArray = ugrid->GetCells();
  const vtkIdType *cells = cellArray->GetPointer();

  // I love this trick: the size of the Links array is equal to
  // the size of the cell array, minus the number of cells.
  this->LinksSize =
    cellArray->GetNumberOfConnectivityEntries() - this->NumCells;

  // Extra one allocated to simplify later pointer manipulation
  this->Links = new TIds[this->LinksSize+1];
  this->Links[this->LinksSize] = this->NumPts;
  this->Offsets = new TIds[this->NumPts+1];
  std::fill_n(this->Offsets, this->NumPts, 0);

  // Now create the links.
  vtkIdType npts, cellId, ptId;
  const vtkIdType *cell=cells;
  int i;

  // Count number of point uses
  for ( cellId=0; cellId < this->NumCells; ++cellId )
  {
    npts = *cell++;
    for (i=0; i<npts; ++i)
    {
      this->Offsets[*cell++]++;
    }
  }

  // Perform prefix sum
  for ( ptId=0; ptId < this->NumPts; ++ptId )
  {
    npts = this->Offsets[ptId+1];
    this->Offsets[ptId+1] = this->Offsets[ptId] + npts;
  }

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for ( cell=cells, cellId=0; cellId < this->NumCells; ++cellId )
  {
    npts = *cell++;
    for (i=0; i<npts; ++i)
    {
      this->Offsets[*cell]--;
      this->Links[this->Offsets[*cell++]] = cellId;
    }
  }
  this->Offsets[this->NumPts] = this->LinksSize;
}

//----------------------------------------------------------------------------
// Build the link list array for poly data. This is more complex because there
// are potentially fout different cell arrays to contend with.
template <typename TIds> void vtkStaticCellLinksTemplate<TIds>::
BuildLinks(vtkPolyData *pd)
{
  // Basic information about the grid
  this->NumCells = pd->GetNumberOfCells();
  this->NumPts = pd->GetNumberOfPoints();

  vtkCellArray *cellArrays[4];
  vtkIdType numCells[4];
  vtkIdType sizes[4];
  int i, j;

  cellArrays[0] = pd->GetVerts();
  cellArrays[1] = pd->GetLines();
  cellArrays[2] = pd->GetPolys();
  cellArrays[3] = pd->GetStrips();

  for (i=0; i<4; ++i)
  {
    if ( cellArrays[i] != NULL )
    {
      numCells[i] = cellArrays[i]->GetNumberOfCells();
      sizes[i] = cellArrays[i]->GetNumberOfConnectivityEntries() - numCells[i];
    }
    else
    {
      numCells[i] = 0;
      sizes[i] = 0;
    }
  }//for the four polydata arrays

  // Allocate
  this->LinksSize = sizes[0] + sizes[1] + sizes[2] + sizes[3];
  this->Links = new TIds[this->LinksSize+1];
  this->Links[this->LinksSize] = this->NumPts;
  this->Offsets = new TIds[this->NumPts+1];
  this->Offsets[this->NumPts] = this->LinksSize;
  std::fill_n(this->Offsets, this->NumPts, 0);

  // Now create the links.
  vtkIdType npts, cellId, CellId, ptId;
  const vtkIdType *cell;

  // Visit the four arrays
  for ( CellId=0, j=0; j < 4; ++j )
  {
    // Count number of point uses
    cell = cellArrays[j]->GetPointer();
    for ( cellId=0; cellId < numCells[j]; ++cellId )
    {
      npts = *cell++;
      for (i=0; i<npts; ++i)
      {
        this->Offsets[CellId+(*cell++)]++;
      }
    }
    CellId += numCells[j];
  } //for each of the four polydata cell arrays

  // Perform prefix sum
  for ( ptId=0; ptId < this->NumPts; ++ptId )
  {
    npts = this->Offsets[ptId+1];
    this->Offsets[ptId+1] = this->Offsets[ptId] + npts;
  }

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for ( CellId=0, j=0; j < 4; ++j )
  {
    cell = cellArrays[j]->GetPointer();
    for ( cellId=0; cellId < numCells[j]; ++cellId )
    {
      npts = *cell++;
      for (i=0; i<npts; ++i)
      {
        this->Offsets[*cell]--;
        this->Links[this->Offsets[*cell++]] = CellId+cellId;
      }
    }
    CellId += numCells[j];
  }//for each of the four polydata arrays
  this->Offsets[this->NumPts] = this->LinksSize;
}

#endif
