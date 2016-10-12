/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLinks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellLinks.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkCellLinks);

//----------------------------------------------------------------------------
vtkCellLinks::~vtkCellLinks()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkCellLinks::Initialize()
{
  if ( this->Array != NULL )
  {
    for (vtkIdType i=0; i<=this->MaxId; i++)
    {
      delete [] this->Array[i].cells;
    }

    delete [] this->Array;
    this->Array = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkCellLinks::Allocate(vtkIdType sz, vtkIdType ext)
{
  static vtkCellLinks::Link linkInit = {0,NULL};

  this->Size = sz;
  delete [] this->Array;
  this->Array = new vtkCellLinks::Link[sz];
  this->Extend = ext;
  this->MaxId = -1;

  for (vtkIdType i=0; i < sz; i++)
  {
    this->Array[i] = linkInit;
  }
}

//----------------------------------------------------------------------------
// Allocate memory for the list of lists of cell ids.
void vtkCellLinks::AllocateLinks(vtkIdType n)
{
  for (vtkIdType i=0; i < n; i++)
  {
    this->Array[i].cells = new vtkIdType[this->Array[i].ncells];
  }
}

//----------------------------------------------------------------------------
// Reclaim any unused memory.
void vtkCellLinks::Squeeze()
{
  this->Resize (this->MaxId+1);
}


//----------------------------------------------------------------------------
void vtkCellLinks::Reset()
{
  this->MaxId = -1;
}

//----------------------------------------------------------------------------
//
// Private function does "reallocate"
//
vtkCellLinks::Link *vtkCellLinks::Resize(vtkIdType sz)
{
  vtkIdType i;
  vtkCellLinks::Link *newArray;
  vtkIdType newSize;
  vtkCellLinks::Link linkInit = {0,NULL};

  if ( sz >= this->Size )
  {
    newSize = this->Size + sz;
  }
  else
  {
    newSize = sz;
  }

  newArray = new vtkCellLinks::Link[newSize];

  for (i=0; i<sz && i<this->Size; i++)
  {
    newArray[i] = this->Array[i];
  }

  for (i=this->Size; i < newSize ; i++)
  {
    newArray[i] = linkInit;
  }

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

//----------------------------------------------------------------------------
// Build the link list array.
void vtkCellLinks::BuildLinks(vtkDataSet *data)
{
  vtkIdType numPts = data->GetNumberOfPoints();
  vtkIdType numCells = data->GetNumberOfCells();
  int j;
  vtkIdType cellId;
  unsigned short *linkLoc;

  // fill out lists with number of references to cells
  linkLoc = new unsigned short[numPts];
  memset(linkLoc, 0, numPts*sizeof(unsigned short));

  // Use fast path if polydata
  if ( data->GetDataObjectType() == VTK_POLY_DATA )
  {
    vtkIdType *pts, npts;

    vtkPolyData *pdata = static_cast<vtkPolyData *>(data);
    // traverse data to determine number of uses of each point
    for (cellId=0; cellId < numCells; cellId++)
    {
      pdata->GetCellPoints(cellId, npts, pts);
      for (j=0; j < npts; j++)
      {
        this->IncrementLinkCount(pts[j]);
      }
    }

    // now allocate storage for the links
    this->AllocateLinks(numPts);
    this->MaxId = numPts - 1;

    for (cellId=0; cellId < numCells; cellId++)
    {
      pdata->GetCellPoints(cellId, npts, pts);
      for (j=0; j < npts; j++)
      {
        this->InsertCellReference(pts[j], (linkLoc[pts[j]])++, cellId);
      }
    }
  }

  else //any other type of dataset
  {
    vtkIdType numberOfPoints, ptId;
    vtkGenericCell *cell=vtkGenericCell::New();

    // traverse data to determine number of uses of each point
    for (cellId=0; cellId < numCells; cellId++)
    {
      data->GetCell(cellId,cell);
      numberOfPoints = cell->GetNumberOfPoints();
      for (j=0; j < numberOfPoints; j++)
      {
        this->IncrementLinkCount(cell->PointIds->GetId(j));
      }
    }

    // now allocate storage for the links
    this->AllocateLinks(numPts);
    this->MaxId = numPts - 1;

    for (cellId=0; cellId < numCells; cellId++)
    {
      data->GetCell(cellId,cell);
      numberOfPoints = cell->GetNumberOfPoints();
      for (j=0; j < numberOfPoints; j++)
      {
        ptId = cell->PointIds->GetId(j);
        this->InsertCellReference(ptId, (linkLoc[ptId])++, cellId);
      }
    }
    cell->Delete();
  }//end else

  delete [] linkLoc;
}

//----------------------------------------------------------------------------
// Build the link list array.
void vtkCellLinks::BuildLinks(vtkDataSet *data, vtkCellArray *Connectivity)
{
  vtkIdType numPts = data->GetNumberOfPoints();
  vtkIdType j, cellId;
  unsigned short *linkLoc;
  vtkIdType npts=0;
  vtkIdType *pts=0;
  vtkIdType loc = Connectivity->GetTraversalLocation();

  // traverse data to determine number of uses of each point
  for (Connectivity->InitTraversal();
       Connectivity->GetNextCell(npts,pts);)
  {
    for (j=0; j < npts; j++)
    {
      this->IncrementLinkCount(pts[j]);
    }
  }

  // now allocate storage for the links
  this->AllocateLinks(numPts);
  this->MaxId = numPts - 1;

  // fill out lists with references to cells
  linkLoc = new unsigned short[numPts];
  memset(linkLoc, 0, numPts*sizeof(unsigned short));

  cellId = 0;
  for (Connectivity->InitTraversal();
       Connectivity->GetNextCell(npts,pts); cellId++)
  {
    for (j=0; j < npts; j++)
    {
      this->InsertCellReference(pts[j], (linkLoc[pts[j]])++, cellId);
    }
  }
  delete [] linkLoc;
  Connectivity->SetTraversalLocation(loc);
}

//----------------------------------------------------------------------------
// Insert a new point into the cell-links data structure. The size parameter
// is the initial size of the list.
vtkIdType vtkCellLinks::InsertNextPoint(int numLinks)
{
  if ( ++this->MaxId >= this->Size )
  {
    this->Resize(this->MaxId + 1);
  }
  this->Array[this->MaxId].cells = new vtkIdType[numLinks];
  return this->MaxId;
}

//----------------------------------------------------------------------------
unsigned long vtkCellLinks::GetActualMemorySize()
{
  vtkIdType size=0;
  vtkIdType ptId;

  for (ptId=0; ptId < (this->MaxId+1); ptId++)
  {
    size += this->GetNcells(ptId);
  }

  size *= sizeof(int *); //references to cells
  size += (this->MaxId+1) * sizeof(vtkCellLinks::Link); //list of cell lists

  return static_cast<unsigned long>( ceil(size/1024.0)); // kibibytes
}

//----------------------------------------------------------------------------
void vtkCellLinks::DeepCopy(vtkCellLinks *src)
{
  this->Allocate(src->Size, src->Extend);
  memcpy(this->Array, src->Array, this->Size * sizeof(vtkCellLinks::Link));
  this->MaxId = src->MaxId;
}

//----------------------------------------------------------------------------
void vtkCellLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend: " << this->Extend << "\n";
}
