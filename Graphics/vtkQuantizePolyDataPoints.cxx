/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantizePolyDataPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuantizePolyDataPoints.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuantizePolyDataPoints, "1.7");
vtkStandardNewMacro(vtkQuantizePolyDataPoints);

//--------------------------------------------------------------------------
// Construct object with initial QFactor of 0.25
vtkQuantizePolyDataPoints::vtkQuantizePolyDataPoints()
{
  this->QFactor   = 0.25;
  this->Tolerance = 0.0;
}

//--------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::OperateOnPoint(float in[3], float out[3])
{
  out[0] = floor(in[0]/this->QFactor + 0.5)*this->QFactor;
  out[1] = floor(in[1]/this->QFactor + 0.5)*this->QFactor;
  out[2] = floor(in[2]/this->QFactor + 0.5)*this->QFactor;
}

//-------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::OperateOnBounds(float in[6], float out[6])
{
  out[0] = floor(in[0]/this->QFactor + 0.5)*this->QFactor;
  out[1] = floor(in[1]/this->QFactor + 0.5)*this->QFactor;
  out[2] = floor(in[2]/this->QFactor + 0.5)*this->QFactor;
  out[3] = floor(in[3]/this->QFactor + 0.5)*this->QFactor;
  out[4] = floor(in[4]/this->QFactor + 0.5)*this->QFactor;
  out[5] = floor(in[5]/this->QFactor + 0.5)*this->QFactor;
}

//--------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "QFactor: " << this->QFactor << "\n";
}

