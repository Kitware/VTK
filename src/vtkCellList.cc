/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellList.cc
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
#include "vtkCellList.hh"

vtkCellList::vtkCellList(const int sz, const int ext)
{
  this->Size = sz;
  this->Array = new _vtkCell_s[sz];
  this->Extend = ext;
  this->MaxId = -1;
}

vtkCellList::~vtkCellList()
{
  delete [] this->Array;
}

// Description:
// Add a cell at specified id.
void vtkCellList::InsertCell(const int cellId, const unsigned char type, const int loc)
{
  _vtkCell_s *cell;

  if ( cellId >= this->Size ) this->Resize(cellId);
  if ( cellId > this->MaxId ) this->MaxId = cellId;

  cell = this->Array + cellId;
  cell->type = type;
  cell->loc = loc;

  return;
}

// Description:
// Add a cell to the object in the next available slot.
int vtkCellList::InsertNextCell(const unsigned char type, const int loc)
{
  this->InsertCell (++this->MaxId,type,loc);
  return this->MaxId;
}

// Description:
// Reclaim any extra memory.
void vtkCellList::Squeeze()
{
  this->Resize (this->MaxId+1);
}

// Description:
// Initialize object without releasing memory.
void vtkCellList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
_vtkCell_s *vtkCellList::Resize(const int sz)
{
  int i;
  _vtkCell_s *newArray;
  int newSize;

  if ( sz >= this->Size )  newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  newArray = new _vtkCell_s[newSize];

  for (i=0; i<sz && i<this->Size; i++)
      newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

