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
#include "vtkgl.h"

vtkStandardNewMacro(vtkContextBufferId);

// ----------------------------------------------------------------------------
vtkContextBufferId::vtkContextBufferId()
{
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
void vtkContextBufferId::SetValues(int srcXmin,
                                   int srcYmin)
{
  assert("pre: is_allocated" && this->IsAllocated());

  GLint savedReadBuffer;
  glGetIntegerv(GL_READ_BUFFER,&savedReadBuffer);

  glReadBuffer(GL_BACK_LEFT);

  // Expensive call here (memory allocation)
  unsigned char *rgb=new unsigned char[this->Width*this->Height*3];

  glPixelStorei(GL_PACK_ALIGNMENT,1);

  // Expensive call here (memory transfer, blocking)
  glReadPixels(srcXmin,srcYmin,this->Width,this->Height,GL_RGB,
               GL_UNSIGNED_BYTE,rgb);

  if(savedReadBuffer!=GL_BACK_LEFT)
    {
    glReadBuffer(static_cast<GLenum>(savedReadBuffer));
    }

  // vtkIntArray
  // Interpret rgb into ids.
  // We cannot just use reinterpret_cast for two reasons:
  // 1. we don't know if the host system is little or big endian.
  // 2. we have rgb, not rgba. if we try to grab rgba and there is not
  // alpha comment, it would be set to 1.0 (255, 0xff). we don't want that.

  // Expensive iteration.
  vtkIdType i=0;
  vtkIdType s=this->Width*this->Height;
  while(i<s)
    {
    vtkIdType j=i*3;
    int value=(static_cast<int>(rgb[j])<<16)|(static_cast<int>(rgb[j+1])<<8)
      |static_cast<int>(rgb[j+2]);
    this->SetValue(i,value);
    ++i;
    }

  delete[] rgb;
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
