/*=========================================================================

  Program:   Visualization Library
  Module:    StrPts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SPoints.hh"

vlStructuredPoints::vlStructuredPoints()
{
  this->Dimension[0] = 0;
  this->Dimension[1] = 0;
  this->Dimension[2] = 0;

  this->AspectRatio[0] = 1.0;
  this->AspectRatio[1] = 1.0;
  this->AspectRatio[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

vlStructuredPoints::vlStructuredPoints(const vlStructuredPoints& v)
{
  this->Dimension[0] = v.Dimension[0];
  this->Dimension[1] = v.Dimension[1];
  this->Dimension[2] = v.Dimension[2];

  this->AspectRatio[0] = v.AspectRatio[0];
  this->AspectRatio[1] = v.AspectRatio[1];
  this->AspectRatio[2] = v.AspectRatio[2];

  this->Origin[0] = v.Origin[0];
  this->Origin[1] = v.Origin[1];
  this->Origin[2] = v.Origin[2];
}

vlStructuredPoints::~vlStructuredPoints()
{
}

int vlStructuredPoints::CellDimension(int cellId)
{
  int i, dim;

  for (dim=3, i=0; i<3; i++)
    {
    if (this->Dimension[i] == 1) dim--;
    if (this->Dimension[i] < 1) return 0;
    }

  return dim;
}

float *vlStructuredPoints::GetPoint(int i)
{
  static float x[3];
  int loc[3];
  
  loc[0] = i % (this->Dimension[0]-1);
  loc[1] = i % ((this->Dimension[0]-1)*(this->Dimension[1]-1))
               / (this->Dimension[0]-1);
  loc[2] = i / ((this->Dimension[0]-1)*(this->Dimension[1]-1));

  for (i=0; i<3; i++)
    x[i] = this->Origin[i] + loc[i] * this->AspectRatio[i];

  return x;
}

void GetPoints(vlIdList& ptId, vlFloatPoints& fp)
{
}

void vlStructuredPoints::CellPoints(int cellId, vlIdList& ptId)
{
}

void vlStructuredPoints::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredPoints::GetClassName()))
    {
    vlDataSet::PrintSelf(os,indent);
    
    os << indent << "Dimension: (" << this->Dimension[0] << ", "
                                    << this->Dimension[1] << ", "
                                    << this->Dimension[2] << ")\n";
    os << indent << "Origin: (" << this->Origin[0] << ", "
                                    << this->Origin[1] << ", "
                                    << this->Origin[2] << ")\n";
    os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", "
                                    << this->AspectRatio[1] << ", "
                                    << this->AspectRatio[2] << ")\n";
    }
}

