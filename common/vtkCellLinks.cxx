/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLinks.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCellLinks.h"
#include "vtkDataSet.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCellLinks* vtkCellLinks::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellLinks");
  if(ret)
    {
    return (vtkCellLinks*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellLinks;
}

void vtkCellLinks::Allocate(vtkIdType sz, vtkIdType ext)
{
  static _vtkLink_s linkInit = {0,NULL};

  this->Size = sz;
  if ( this->Array != NULL )
    {
    delete [] this->Array;
    }
  this->Array = new _vtkLink_s[sz];
  this->Extend = ext;
  this->MaxId = -1;

  for (vtkIdType i=0; i < sz; i++)
    {
    this->Array[i] = linkInit;
    }
}

vtkCellLinks::~vtkCellLinks()
{
  if ( this->Array == NULL )
    {
    return;
    }

  for (vtkIdType i=0; i<=this->MaxId; i++)
    {
    if ( this->Array[i].cells != NULL )
      {
      delete [] this->Array[i].cells;
      }
    }

  delete [] this->Array;
}

// Allocate memory for the list of lists of cell ids.
void vtkCellLinks::AllocateLinks(int n)
{
  for (int i=0; i < n; i++)
    {
    this->Array[i].cells = new vtkIdType[this->Array[i].ncells];
    }
}

// Reclaim any unused memory.
void vtkCellLinks::Squeeze()
{
  this->Resize (this->MaxId+1);
}


void vtkCellLinks::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
_vtkLink_s *vtkCellLinks::Resize(vtkIdType sz)
{
  vtkIdType i;
  _vtkLink_s *newArray;
  vtkIdType newSize;
  _vtkLink_s linkInit = {0,NULL};

  if ( sz >= this->Size )
    {
    newSize = this->Size + sz;
    }
  else
    {
    newSize = sz;
    }

  newArray = new _vtkLink_s[newSize];

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
    
    vtkPolyData *pdata = (vtkPolyData *)data;
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

// Build the link list array.
void vtkCellLinks::BuildLinks(vtkDataSet *data, vtkCellArray *Connectivity)
{
  vtkIdType numPts = data->GetNumberOfPoints();
  vtkIdType j, cellId;
  unsigned short *linkLoc;
  vtkIdType npts;
  vtkIdType *pts;
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

unsigned long vtkCellLinks::GetActualMemorySize()
{
  unsigned long size=0;
  vtkIdType ptId;

  for (ptId=0; ptId < (this->MaxId+1); ptId++)
    {
    size += this->GetNcells(ptId);
    }

  size *= sizeof(int *); //references to cells
  size += (this->MaxId+1) * sizeof(_vtkLink_s); //list of cell lists

  return (unsigned long) ceil((float)size/1000.0); //kilobytes
}

void vtkCellLinks::DeepCopy(vtkCellLinks *src)
{
  this->Allocate(src->Size, src->Extend);
  memcpy(this->Array, src->Array, this->Size * sizeof(_vtkLink_s));
  this->MaxId = src->MaxId;
}

