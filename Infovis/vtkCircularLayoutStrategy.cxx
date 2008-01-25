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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkCircularLayoutStrategy.h"

#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

vtkCxxRevisionMacro(vtkCircularLayoutStrategy, "1.2");
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

