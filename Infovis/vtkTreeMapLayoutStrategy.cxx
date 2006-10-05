/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeMapLayoutStrategy.h"

vtkCxxRevisionMacro(vtkTreeMapLayoutStrategy, "1.1");

vtkTreeMapLayoutStrategy::vtkTreeMapLayoutStrategy():BorderPercentage(0.0)
{
}

vtkTreeMapLayoutStrategy::~vtkTreeMapLayoutStrategy()
{
}

void vtkTreeMapLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "BorderPercentage = " << this->BorderPercentage << endl;
}

void vtkTreeMapLayoutStrategy::AddBorder(float *boxInfo) 
{
  float dx, dy;
  dx = 0.5 * (boxInfo[1] - boxInfo[0]) * this->BorderPercentage;
  dy = 0.5 * (boxInfo[3] - boxInfo[2]) * this->BorderPercentage;
  boxInfo[0] += dx;
  boxInfo[1] -= dx;
  boxInfo[2] += dy;
  boxInfo[3] -= dy;
}
  
