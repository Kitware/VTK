/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensor.h
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
// .NAME vtkTensor - supporting class to enable assignment and referencing of tensors
// .SECTION Description
// vtkTensor is a floating point representation of an nxn tensor. vtkTensor 
// provides methods for assignment and reference of tensor components. It 
// does it in such a way as to minimize data copying.
// .SECTION Caveats
// vtkTensor performs its operations using pointer reference. You are 
// responsible for supplying data storage (if necessary) if local copies 
// of data are being made.

#ifndef __vtkTensor_h
#define __vtkTensor_h

#define VTK_TENSOR_MAXDIM 3
#include "vtkWin32Header.h"

class VTK_EXPORT vtkTensor
{
public:
  vtkTensor(int dim=3);
  void Initialize();
  float GetComponent(int i, int j);
  void SetComponent(int i, int j, float v);
  void AddComponent(int i, int j, float v);
  float *GetColumn(int j);
  void operator=(float *t);
  void operator=(vtkTensor &t);
  operator float*() {return this->T;};
  void SetDimension(int dim);
  int GetDimension();
  float *T;

protected: 
  int Dimension;
  float Storage[VTK_TENSOR_MAXDIM*VTK_TENSOR_MAXDIM];
};

// Description:
// Construct tensor initially pointing to internal storage.
inline vtkTensor::vtkTensor(int dim)
{
  this->T = this->Storage;
  this->Dimension = dim;
  for (int j=0; j<this->Dimension; j++)
    for (int i=0; i<this->Dimension; i++)
      this->T[i+j*this->Dimension] = 0.0;
}

// Description:
// Set the dimensions of the tensor.
inline void vtkTensor::SetDimension(int dim)
{
  this->Dimension = (dim < 1 ? 1 : (dim > VTK_TENSOR_MAXDIM ? VTK_TENSOR_MAXDIM : dim));
}

// Description:
// Get the dimensions of the tensor.
inline int vtkTensor::GetDimension()
{
  return this->Dimension;
}

// Description:
// Initialize tensor components to 0.0.
inline void vtkTensor::Initialize()
{
  for (int j=0; j<this->Dimension; j++)
    for (int i=0; i<this->Dimension; i++)
      this->T[i+j*this->Dimension] = 0.0;
}

// Description:
// Get the tensor component (i,j).
inline float vtkTensor::GetComponent(int i, int j)
{
  return this->T[i+this->Dimension*j];
}

// Description:
// Set the value of the tensor component (i,j).
inline void vtkTensor::SetComponent(int i, int j, float v)
{
  this->T[i+this->Dimension*j] = v;
}

// Description:
// Add to the value of the tensor component at location (i,j).
inline void vtkTensor::AddComponent(int i, int j, float v)
{
  this->T[i+this->Dimension*j] += v;
}

// Description:
// Assign tensors to a float array. Float array must be sized
// Dimension*Dimension.
inline void vtkTensor::operator=(float *t)
{
  for (int j=0; j < this->Dimension; j++)
    for (int i=0; i < this->Dimension; i++)
      this->T[i+this->Dimension*j] = t[i+this->Dimension*j];
}

// Description:
// Assign tensor to another tensor.
inline void vtkTensor::operator=(vtkTensor &t)
{
  for (int j=0; j < this->Dimension; j++)
    for (int i=0; i < this->Dimension; i++)
      this->T[i+this->Dimension*j] = t.T[i+this->Dimension*j];
}

// Description:
// Return column vector from tensor. (Assumes 2D matrix form and 0-offset.)
inline float *vtkTensor::GetColumn(int j)
{
  return this->T + this->Dimension*j;
}

#endif
