/*=========================================================================

  Program:   Visualization Library
  Module:    FTensors.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FTensors.hh"

vlTensor &vlFloatTensors::GetTensor(int i) 
{
  static vlTensor t;
  t.SetDimension(this->Dimension);

  t.T = this->T.GetPtr(this->Dimension*this->Dimension*i);
  return t;
}

vlTensors *vlFloatTensors::MakeObject(int sze, int d, int ext)
{
  return new vlFloatTensors(sze,d,ext);
}

// Description:
// Deep copy of texture coordinates.
vlFloatTensors& vlFloatTensors::operator=(const vlFloatTensors& ft)
{
  this->T = ft.T;
  this->Dimension = ft.Dimension;
  
  return *this;
}

