/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Tensor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

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

#define MAXDIM 3

class vtkTensor
{
public:
  vtkTensor(int dim=3);
  void Initialize();
  float GetComponent(int i, int j);
  void SetComponent(int i, int j, float v);
  void AddComponent(int i, int j, float v);
  void operator=(float *t);
  void operator=(vtkTensor &t);
  operator float*() {return this->T;};
  void SetDimension(int dim);
  int GetDimension();
  float *T;

protected: 
  int Dimension;
  float Storage[MAXDIM*MAXDIM];
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
  this->Dimension = (dim < 1 ? 1 : (dim > MAXDIM ? MAXDIM : dim));
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

#endif
