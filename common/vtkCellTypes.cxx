/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTypes.cxx
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
#include "vtkCellTypes.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCellTypes* vtkCellTypes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellTypes");
  if(ret)
    {
    return (vtkCellTypes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellTypes;
}




vtkCellTypes::~vtkCellTypes()
{
  delete [] this->Array;
}

// Allocate memory for this array. Delete old storage only if necessary.
int vtkCellTypes::Allocate(int sz, int ext)
{
  if ( sz > this->Size || this->Array == NULL )
    {
    if ( this->Array != NULL )
      {
      delete [] this->Array;
      }

    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new _vtkCell_s[this->Size]) == NULL )
      {
      return 0;
      }
    }

  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Add a cell at specified id.
void vtkCellTypes::InsertCell(int cellId, unsigned char type, int loc)
{
  _vtkCell_s *cell;

  if ( cellId >= this->Size )
    {
    this->Resize(cellId + 1);
    }
  if ( cellId > this->MaxId )
    {
    this->MaxId = cellId;
    }

  cell = this->Array + cellId;
  cell->type = type;
  cell->loc = loc;

  return;
}

// Add a cell to the object in the next available slot.
int vtkCellTypes::InsertNextCell(unsigned char type, int loc)
{
  this->InsertCell (++this->MaxId,type,loc);
  return this->MaxId;
}

// Reclaim any extra memory.
void vtkCellTypes::Squeeze()
{
  this->Resize (this->MaxId+1);
}

// Initialize object without releasing memory.
void vtkCellTypes::Reset()
{
  this->MaxId = -1;
}

// Private function does "reallocate"
//
_vtkCell_s *vtkCellTypes::Resize(int sz)
{
  int i;
  _vtkCell_s *newArray;
  int newSize;

  if ( sz >= this->Size )
    {
    newSize = this->Size + sz;
    }
  else
    {
    newSize = sz;
    }

  newArray = new _vtkCell_s[newSize];

  for (i=0; i<sz && i<this->Size; i++)
    {
    newArray[i] = this->Array[i];
    }

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}

unsigned long vtkCellTypes::GetActualMemorySize()
{
  unsigned long size=sizeof(_vtkCell_s)*this->GetNumberOfTypes();

  return (unsigned long) ceil((float)size/1000.0); //kilobytes
}


void vtkCellTypes::DeepCopy(vtkCellTypes *src)
{
  this->Allocate(src->Size, src->Extend);
  memcpy(this->Array, src->Array, this->Size * sizeof(_vtkCell_s));
  this->MaxId = src->MaxId;
}

