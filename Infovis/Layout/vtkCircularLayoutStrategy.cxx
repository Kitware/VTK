/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCircularLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkCircularLayoutStrategy.h"

#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkCircularLayoutStrategy);

vtkCircularLayoutStrategy::vtkCircularLayoutStrategy()
{
}

vtkCircularLayoutStrategy::~vtkCircularLayoutStrategy()
{
}

void vtkCircularLayoutStrategy::Layout()
{
  vtkPoints* points = vtkPoints::New();
  vtkIdType numVerts = this->Graph->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for (vtkIdType i = 0; i < numVerts; i++)
  {
    double x = cos(2.0*vtkMath::Pi()*i/numVerts);
    double y = sin(2.0*vtkMath::Pi()*i/numVerts);
    points->SetPoint(i, x, y, 0);
  }
  this->Graph->SetPoints(points);
  points->Delete();
}

void vtkCircularLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

