/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellArray.cxx
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
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCellArray* vtkCellArray::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellArray");
  if(ret)
    {
    return (vtkCellArray*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellArray;
}




vtkCellArray::vtkCellArray()
{
  this->Ia = vtkIntArray::New();
  this->NumberOfCells = 0;
  this->InsertLocation = 0;
  this->TraversalLocation = 0;
}

vtkCellArray::vtkCellArray(const int sz, const int ext)
{
  this->Ia = vtkIntArray::New();
  this->Ia->Allocate(sz,ext);
  this->NumberOfCells = 0;
  this->InsertLocation = 0;
  this->TraversalLocation = 0;
}

void vtkCellArray::DeepCopy (vtkCellArray *ca)
{
  this->Ia->DeepCopy(ca->Ia);
  this->NumberOfCells = ca->NumberOfCells;
  this->InsertLocation = 0;
  this->TraversalLocation = 0;
}

vtkCellArray::~vtkCellArray()
{
  this->Ia->Delete();
}


// Returns the size of the largest cell. The size is the number of points
// defining the cell.
int vtkCellArray::GetMaxCellSize()
{
  int i, npts=0, maxSize=0;

  for (i=0; i<this->Ia->GetMaxId(); i+=(npts+1))
    {
    if ( (npts=this->Ia->GetValue(i)) > maxSize )
      {
      maxSize = npts;
      }
    }
  return maxSize;
}

// Specify a group of cells.
void vtkCellArray::SetCells(int ncells, vtkIntArray *cells)
{
  if ( cells != this->Ia )
    {
    this->Modified();
    this->Ia->Delete();
    this->Ia = cells;
    this->Ia->Register(this);

    this->NumberOfCells = ncells;
    this->InsertLocation = cells->GetMaxId() + 1;
    this->TraversalLocation = 0;
    }
}

unsigned long vtkCellArray::GetActualMemorySize()
{
  return this->Ia->GetActualMemorySize();
}
