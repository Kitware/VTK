/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensors.cxx
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
#include "vtkTensors.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkTensors* vtkTensors::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTensors");
  if(ret)
    {
    return (vtkTensors*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTensors;
}





vtkTensors *vtkTensors::New(int dataType)
{
  vtkTensors *res = vtkTensors::New();
  res->SetDataType(dataType);
  res->GetData()->SetNumberOfComponents(9);
  return res;
}

// Construct object with an initial data array of type float.
vtkTensors::vtkTensors()
{
  this->Data->SetNumberOfComponents(9);
  this->T = vtkTensor::New();
}

vtkTensors::~vtkTensors()
{
  this->T->Delete();
}


void vtkTensors::GetTensor(vtkIdType id, vtkTensor *ft)
{
  vtkTensor *t = this->GetTensor(id);
  ft->DeepCopy(t);
}

void vtkTensors::SetTensor(vtkIdType id, vtkTensor *t)
{
  this->Data->SetTuple(id, t->T);
}

void vtkTensors::InsertTensor(vtkIdType id, vtkTensor *t)
{
  this->Data->InsertTuple(id, t->T);
}

void vtkTensors::InsertTensor(vtkIdType id, float t11, float t12, float t13, 
                              float t21, float t22, float t23, 
                              float t31, float t32, float t33)
{
  vtkTensor *t = vtkTensor::New();
  t->SetComponent(0,0,t11);
  t->SetComponent(0,1,t12);
  t->SetComponent(0,2,t13);
  t->SetComponent(1,0,t21);
  t->SetComponent(1,1,t22);
  t->SetComponent(1,2,t23);
  t->SetComponent(2,0,t31);
  t->SetComponent(2,1,t32);
  t->SetComponent(2,2,t33);

  this->InsertTensor(id,t);
  t->Delete();
}

vtkIdType vtkTensors::InsertNextTensor(vtkTensor *t)
{
  return this->Data->InsertNextTuple(t->T);
}

vtkIdType vtkTensors::InsertNextTensor(float t11, float t12, float t13, 
                                       float t21, float t22, float t23, 
                                       float t31, float t32, float t33)
{
  vtkTensor *t = vtkTensor::New();
  t->SetComponent(0,0,t11);
  t->SetComponent(0,1,t12);
  t->SetComponent(0,2,t13);
  t->SetComponent(1,0,t21);
  t->SetComponent(1,1,t22);
  t->SetComponent(1,2,t23);
  t->SetComponent(2,0,t31);
  t->SetComponent(2,1,t32);
  t->SetComponent(2,2,t33);

  vtkIdType id = this->InsertNextTensor(t);
  t->Delete();
  return id;
}

// Given a list of pt ids, return an array of tensors.
void vtkTensors::GetTensors(vtkIdList *ptIds, vtkTensors *t)
{
  vtkIdType num=ptIds->GetNumberOfIds();

  t->SetNumberOfTensors(num);
  for (vtkIdType i=0; i<num; i++)
    {
    t->SetTensor(i,this->GetTensor(ptIds->GetId(i)));
    }
}

void vtkTensors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkAttributeData::PrintSelf(os,indent);

  os << indent << "Number Of Tensors: " << this->GetNumberOfTensors() << "\n";
}
