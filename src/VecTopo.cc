/*=========================================================================

  Program:   Visualization Library
  Module:    VecTopo.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "VecTopo.hh"
#include "vtkMath.hh"

vtkVectorTopology::vtkVectorTopology()
{
  this->Distance = 0.1;
}

vtkVectorTopology::~vtkVectorTopology()
{
  this->Distance = 0.1;
}

void vtkVectorTopology::Execute()
{
  vtkDataSet *input=this->Input;
  vtkVectors *inVectors;
  vtkFloatPoints *newPts;
  vtkCellArray *newVerts;

  vtkDebugMacro(<< "Executing vector topology...");
//
// Initialize self; check input; create output objects
//
  this->Initialize();

  // make sure we have vector data
  if ( ! (inVectors = input->GetPointData()->GetVectors()) )
    {
    vtkErrorMacro(<<"No vector data to contour");
    return;
    }

  vtkDebugMacro(<< "Created " << newPts->GetNumberOfPoints() << "points");
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->SetVerts(newVerts);
  this->Squeeze();
}

void vtkVectorTopology::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "Distance: " << this->Distance << "\n";
}


