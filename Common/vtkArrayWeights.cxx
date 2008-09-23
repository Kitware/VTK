/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayWeights.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayWeights.h"

vtkArrayWeights::vtkArrayWeights()
{
}

vtkArrayWeights::vtkArrayWeights(double i) :
  Storage(1)
{
  this->Storage[0] = i;
}

vtkArrayWeights::vtkArrayWeights(double i, double j) :
  Storage(2)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
}

vtkArrayWeights::vtkArrayWeights(double i, double j, double k) :
  Storage(3)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
}

vtkArrayWeights::vtkArrayWeights(double i, double j, double k, double l) :
  Storage(4)
{
  this->Storage[0] = i;
  this->Storage[1] = j;
  this->Storage[2] = k;
  this->Storage[3] = l;
}

const vtkIdType vtkArrayWeights::GetCount() const
{
  return this->Storage.size();
}

void vtkArrayWeights::SetCount(vtkIdType count)
{
  this->Storage.assign(count, 0.0);
}

double& vtkArrayWeights::operator[](vtkIdType i)
{
  return this->Storage[i];
}

const double& vtkArrayWeights::operator[](vtkIdType i) const
{
  return this->Storage[i];
}

