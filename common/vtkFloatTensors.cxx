/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatTensors.cxx
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
#include "vtkFloatTensors.h"

vtkFloatTensors::vtkFloatTensors()
{
  this->T = new vtkFloatArray;
}

vtkFloatTensors::vtkFloatTensors(const vtkFloatTensors& ft)
{
  this->T = new vtkFloatArray;
  *(this->T) = *(ft.T);
  this->Dimension = ft.Dimension;
}

vtkFloatTensors::vtkFloatTensors(int sz, int d=3, int ext=1000)
{
  this->T = new vtkFloatArray(d*d*sz,d*d*ext);
  this->Dimension = d;
}

vtkFloatTensors::~vtkFloatTensors()
{
  this->T->Delete();
}





vtkTensor *vtkFloatTensors::GetTensor(int i) 
{
  static vtkTensor t;
  t.SetDimension(this->Dimension);

  t.T = this->T->GetPtr(this->Dimension*this->Dimension*i);
  return &t;
}

vtkTensors *vtkFloatTensors::MakeObject(int sze, int d, int ext)
{
  return new vtkFloatTensors(sze,d,ext);
}

// Description:
// Deep copy of tensors.
vtkFloatTensors& vtkFloatTensors::operator=(const vtkFloatTensors& ft)
{
  *(this->T) = *(ft.T);
  this->Dimension = ft.Dimension;
  
  return *this;
}

void vtkFloatTensors::SetTensor(int id, vtkTensor *t) 
{
  id *= this->Dimension*this->Dimension; 
  
  for (int j=0; j < this->Dimension; j++) 
    for (int i=0; i < this->Dimension; i++) 
      this->T->SetValue(id+i+t->GetDimension()*j, t->GetComponent(i,j));
}

void vtkFloatTensors::InsertTensor(int id, vtkTensor *t) 
{
  id *= this->Dimension*this->Dimension; 
  
  for (int j=0; j < this->Dimension; j++) 
    for (int i=0; i < this->Dimension; i++) 
      this->T->InsertValue(id+i+t->GetDimension()*j,t->GetComponent(i,j));
}

int vtkFloatTensors::InsertNextTensor(vtkTensor *t) 
{
  int id = this->GetNumberOfTensors() + 1;
  for (int j=0; j < this->Dimension; j++) 
    for (int i=0; i < this->Dimension; i++) 
      this->T->InsertNextValue(t->GetComponent(i,j));

  return id;
}
