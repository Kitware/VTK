/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensors.cc
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
#include "vtkTensors.hh"

vtkTensors::vtkTensors(int dim)
{
  this->Dimension = dim;
}

void vtkTensors::GetTensor(int id, vtkTensor &ft)
{
  vtkTensor *t = this->GetTensor(id);
  ft = *t;
}

void vtkTensors::InsertTensor(int id, float t11, float t12, float t13, 
                              float t21, float t22, float t23, 
                              float t31, float t32, float t33)
{
  vtkTensor t;
  t.SetComponent(0,0,t11);
  t.SetComponent(0,1,t12);
  t.SetComponent(0,2,t13);
  t.SetComponent(1,0,t21);
  t.SetComponent(1,1,t22);
  t.SetComponent(1,2,t23);
  t.SetComponent(2,0,t31);
  t.SetComponent(2,1,t32);
  t.SetComponent(2,2,t33);

  this->InsertTensor(id,&t);
}

int vtkTensors::InsertNextTensor(float t11, float t12, float t13, 
                                 float t21, float t22, float t23, 
                                 float t31, float t32, float t33)
{
  vtkTensor t;
  t.SetComponent(0,0,t11);
  t.SetComponent(0,1,t12);
  t.SetComponent(0,2,t13);
  t.SetComponent(1,0,t21);
  t.SetComponent(1,1,t22);
  t.SetComponent(1,2,t23);
  t.SetComponent(2,0,t31);
  t.SetComponent(2,1,t32);
  t.SetComponent(2,2,t33);

  return this->InsertNextTensor(&t);
}

// Description:
// Given a list of pt ids, return an array of tensors.
void vtkTensors::GetTensors(vtkIdList& ptId, vtkFloatTensors& ft)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ft.InsertTensor(i,this->GetTensor(ptId.GetId(i)));
    }
}

void vtkTensors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Tensors: " << this->GetNumberOfTensors() << "\n";
}
