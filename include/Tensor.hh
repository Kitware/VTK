/*=========================================================================

  Program:   Visualization Library
  Module:    Tensor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTensor - supporting class to enable assignment and referencing of tensors
// .SECTION Description
// vlTensor is a floating point representation of an nxn tensor. vlTensor 
// provides methods for assignment and reference of tensor components. It 
// does it in such a way as to minimize data copying.
// .SECTION Caveats
// vlTensor performs its operations using pointer reference. You are 
// responsible for supplying data storage (if necessary) if local copies 
// of data are being made.

#ifndef __vlTensor_h
#define __vlTensor_h

#define MAXDIM 3

class vlTensor
{
public:
  vlTensor(int dim=3);
  void Initialize();
  float GetComponent(int i, int j);
  void SetComponent(int i, int j, float v);
  void AddComponent(int i, int j, float v);
  void operator=(float *t);
  void operator=(vlTensor &t);
  operator float*() {return this->T;};
  void SetDimension(int dim);
  int GetDimension();
  float *T;

protected: 
  int Dimension;
  float Storage[MAXDIM*MAXDIM];
};

inline vlTensor::vlTensor(int dim)
{
  this->T = this->Storage;
  this->Dimension = dim;
  for (int j=0; j<this->Dimension; j++)
    for (int i=0; i<this->Dimension; i++)
      this->T[i+j*this->Dimension] = 0.0;
}

inline void vlTensor::SetDimension(int dim)
{
  this->Dimension = (dim < 1 ? 1 : (dim > MAXDIM ? MAXDIM : dim));
}

inline int vlTensor::GetDimension()
{
  return this->Dimension;
}

inline void vlTensor::Initialize()
{
  for (int j=0; j<this->Dimension; j++)
    for (int i=0; i<this->Dimension; i++)
      this->T[i+j*this->Dimension] = 0.0;
}

inline float vlTensor::GetComponent(int i, int j)
{
  return this->T[i+this->Dimension*j];
}

inline void vlTensor::SetComponent(int i, int j, float v)
{
  this->T[i+this->Dimension*j] = v;
}

inline void vlTensor::AddComponent(int i, int j, float v)
{
  this->T[i+this->Dimension*j] += v;
}

inline void vlTensor::operator=(float *t)
{
  for (int j=0; j < this->Dimension; j++)
    for (int i=0; i < this->Dimension; i++)
      this->T[i+this->Dimension*j] = t[i+this->Dimension*j];
}

inline void vlTensor::operator=(vlTensor &t)
{
  for (int j=0; j < this->Dimension; j++)
    for (int i=0; i < this->Dimension; i++)
      this->T[i+this->Dimension*j] = t.T[i+this->Dimension*j];
}

#endif
