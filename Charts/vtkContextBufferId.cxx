/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextBufferId.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextBufferId.h"

#include "vtkIntArray.h"
#include <cassert>
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkContextBufferId);

// ----------------------------------------------------------------------------
vtkContextBufferId::vtkContextBufferId()
{
  this->Width=0;
  this->Height=0;
  this->IdArray=0;
}

// ----------------------------------------------------------------------------
vtkContextBufferId::~vtkContextBufferId()
{
  if(this->IdArray!=0)
    {
    this->IdArray->Delete();
    }
}

// ----------------------------------------------------------------------------
void vtkContextBufferId::Allocate()
{
  assert("pre: positive_width" && this->GetWidth()>0);
  assert("pre: positive_height" && this->GetHeight()>0);

  vtkIdType size=this->Width*this->Height;
  if(this->IdArray!=0 && this->IdArray->GetNumberOfTuples()<size)
    {
    this->IdArray->Delete();
    this->IdArray=0;
    }
  if(this->IdArray==0)
    {
    this->IdArray=vtkIntArray::New(); // limit to 32-bit
    this->IdArray->SetNumberOfComponents(1);
    this->IdArray->SetNumberOfTuples(size); // allocation
    }
}

// ----------------------------------------------------------------------------
bool vtkContextBufferId::IsAllocated() const
{

  return this->IdArray!=0 &&
    this->IdArray->GetNumberOfTuples()>=(this->Width*this->Height);
}

// ----------------------------------------------------------------------------
void vtkContextBufferId::SetValue(vtkIdType i,
                                  int value)
{
  assert("pre: is_allocated" && this->IsAllocated());
  assert("pre: valid_i" && i>=0 && i<this->GetWidth()*this->GetHeight());

  this->IdArray->SetValue(i,value);

  assert("post: is_set" && this->GetValue(i)==value);
}

// ----------------------------------------------------------------------------
int vtkContextBufferId::GetValue(vtkIdType i)
{
  assert("pre: is_allocated" && this->IsAllocated());
  assert("pre: valid_i" && i>=0 && i<this->GetWidth()*this->GetHeight());

  return this->IdArray->GetValue(i);
}

// ----------------------------------------------------------------------------
vtkIdType vtkContextBufferId::GetPickedItem(int x, int y)
{
  assert("pre: is_allocated" && this->IsAllocated());

  vtkIdType result=-1;
  if(x<0 || x>=this->Width)
    {
    vtkDebugMacro(<<"x mouse position out of range: x=" << x << " (width="
                    << this->Width <<")");
    }
  else
    {
    if(y<0 || y>=this->Height)
      {
      vtkDebugMacro(<<"y mouse position out of range: y="<< y << " (height="
                      << this->Height << ")");
      }
    else
      {
      result=
        static_cast<vtkIdType>(this->IdArray->GetValue(y*this->Width+x))-1;
      }
    }

  assert("post: valid_result" && result>=-1 );
  return result;
}

//-----------------------------------------------------------------------------
void vtkContextBufferId::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
