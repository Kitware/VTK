/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkList.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkLinkList.hh"
#include "vtkDataSet.hh"

vtkLinkList::vtkLinkList(int sz, int ext)
{
  static _vtkLink_s linkInit = {0,NULL};

  this->Size = sz;
  this->Array = new _vtkLink_s[sz];
  this->Extend = ext;
  this->MaxId = -1;

  for (int i=0; i < sz; i++) this->Array[i] = linkInit;
}

vtkLinkList::~vtkLinkList()
{
  if ( this->Array == NULL ) return;

  for (int i=0; i<=this->MaxId; i++)
    {
    if ( this->Array[i].cells != NULL ) 
      delete [] this->Array[i].cells;
    }

  delete [] this->Array;
}

// Description:
// Allocate memory for the list of lists of cell ids.
void vtkLinkList::AllocateLinks(int n)
{
  for (int i=0; i < n; i++)
    {
    this->Array[i].cells = new int[this->Array[i].ncells];
    }
}

// Description:
// Reclaim any unused memory.
void vtkLinkList::Squeeze()
{
  this->Resize (this->MaxId+1);
}


void vtkLinkList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
_vtkLink_s *vtkLinkList::Resize(int sz)
{
  int i;
  _vtkLink_s *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new _vtkLink_s[newSize];

  for (i=0; i<sz && i<this->Size; i++)
    newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Description:
// Build the link list array.
void vtkLinkList::BuildLinks(vtkDataSet *data)
{
  int numPts = data->GetNumberOfPoints();
  int numCells = data->GetNumberOfCells();
  int i, j, ptId, cellId;
  vtkCell *cell;
  unsigned short *linkLoc;

  // traverse data to determine number of uses of each point
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = data->GetCell(cellId);
    for (j=0; j < cell->GetNumberOfPoints(); j++)
      {
      this->IncrementLinkCount(cell->PointIds.GetId(j));      
      }      
    }

  // now allocate storage for the links
  this->AllocateLinks(numPts);
  this->MaxId = numPts - 1;

  // fill out lists with references to cells
  linkLoc = new unsigned short[numPts];
  for (i=0; i < numPts; i++) linkLoc[i] = 0;

  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = data->GetCell(cellId);
    for (j=0; j < cell->GetNumberOfPoints(); j++)
      {
      ptId = cell->PointIds.GetId(j);
      this->InsertCellReference(ptId, (linkLoc[ptId])++, cellId);      
      }      
    }

  delete [] linkLoc;
}

