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

vtkCellTypes::vtkCellTypes ()
{

  this->TypeArray = NULL;
  this->LocationArray = NULL;
  this->Size = 0;
  this->MaxId = -1;
  this->Extend = 1000;
  this->Allocate(1000,this->Extend);

}

vtkCellTypes::~vtkCellTypes()
{

  if ( this->TypeArray )
    {
    this->TypeArray->UnRegister(this);
    this->TypeArray = NULL;
    }

  if ( this->LocationArray )
    {
    this->LocationArray->UnRegister(this);
    this->LocationArray = NULL;
    }

}

// Allocate memory for this array. Delete old storage only if necessary.
int vtkCellTypes::Allocate(int sz, int ext)
{

  this->Size = ( sz > 0 ? sz : 1);
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  if ( this->TypeArray )
    {
    this->TypeArray->UnRegister(this);
    }
  this->TypeArray = vtkUnsignedCharArray::New();
  this->TypeArray->Allocate(sz,ext);
  this->TypeArray->Register(this);
  this->TypeArray->Delete();

  if ( this->LocationArray )
    {
    this->LocationArray->UnRegister(this);
    }
  this->LocationArray = vtkIntArray::New();
  this->LocationArray->Allocate(sz,ext);
  this->LocationArray->Register(this);
  this->LocationArray->Delete();

  return 1;
}

// Add a cell at specified id.
void vtkCellTypes::InsertCell(int cellId, unsigned char type, int loc)
{
  vtkDebugMacro(<<"Insert Cell id: " << cellId << " at location " << loc);
  TypeArray->InsertValue(cellId, type);

  LocationArray->InsertValue(cellId, loc);

  if ( cellId > this->MaxId )
    {
    this->MaxId = cellId;
    }
  return;
}

// Add a cell to the object in the next available slot.
int vtkCellTypes::InsertNextCell(unsigned char type, int loc)
{
  vtkDebugMacro(<<"Insert Next Cell " << type << " location " << loc);
  this->InsertCell (++this->MaxId,type,loc);
  return this->MaxId;
}

// Specify a group of cell types.
void vtkCellTypes::SetCellTypes(int ncells, vtkUnsignedCharArray *cellTypes, vtkIntArray *cellLocations)
{
  this->Size = ncells;
  this->TypeArray = cellTypes;
  this->LocationArray = cellLocations;
  this->Extend = 1;
  this->MaxId = -1;

}

// Reclaim any extra memory.
void vtkCellTypes::Squeeze()
{
  this->TypeArray->Squeeze();
  this->LocationArray->Squeeze();
}

// Initialize object without releasing memory.
void vtkCellTypes::Reset()
{
  this->MaxId = -1;
}

unsigned long vtkCellTypes::GetActualMemorySize()
{
  unsigned long size=0;

  if ( this->TypeArray )
    {
    size += this->TypeArray->GetActualMemorySize();
    }

  if ( this->LocationArray )
    {
    size += this->LocationArray->GetActualMemorySize();
    }

  return (unsigned long) ceil((float)size/1000.0); //kilobytes
}


void vtkCellTypes::DeepCopy(vtkCellTypes *src)
{
  if (this->TypeArray)
    {
      this->TypeArray->UnRegister(this);
      this->TypeArray = NULL;
    }
  if (src->TypeArray)
    {
      this->TypeArray = vtkUnsignedCharArray::New();
      this->TypeArray->DeepCopy(src->TypeArray);
      this->TypeArray->Register(this);
      this->TypeArray->Delete();
    }

  if (this->LocationArray)
    {
      this->LocationArray->UnRegister(this);
      this->LocationArray = NULL;
    }
  if (src->LocationArray)
    {
      this->LocationArray = vtkIntArray::New();
      this->LocationArray->DeepCopy(src->LocationArray);
      this->LocationArray->Register(this);
      this->LocationArray->Delete();
    }

  this->Allocate(src->Size, src->Extend);
  this->MaxId = src->MaxId;
}

