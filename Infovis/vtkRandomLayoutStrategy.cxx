/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomLayoutStrategy.cxx

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

#include "vtkRandomLayoutStrategy.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDataArray.h>

#include "vtkTree.h"

vtkCxxRevisionMacro(vtkRandomLayoutStrategy, "1.3");
vtkStandardNewMacro(vtkRandomLayoutStrategy);

vtkRandomLayoutStrategy::vtkRandomLayoutStrategy()
{
  this->GraphBounds[0] = this->GraphBounds[2] = this->GraphBounds[4] = -0.5;
  this->GraphBounds[1] = this->GraphBounds[3] = this->GraphBounds[5] =  0.5;
  this->AutomaticBoundsComputation = 0;
  this->ThreeDimensionalLayout = 1;
}

vtkRandomLayoutStrategy::~vtkRandomLayoutStrategy()
{
}

// Random graph layout method
// Fixme: Temporary Hack
void vtkRandomLayoutStrategy::Layout() {}; 
void vtkRandomLayoutStrategy::SetGraph(vtkAbstractGraph *graph)
{
  if (graph == NULL)
    {
    return;
    }

  // Generate bounds automatically if necessary. It's the same
  // as the graph bounds.
  if ( this->AutomaticBoundsComputation )
    {
    vtkPoints* pts = graph->GetPoints();
    pts->GetBounds(this->GraphBounds);
    }

  for (int i=0; i<3; i++)
    {
    if ( this->GraphBounds[2*i+1] <= this->GraphBounds[2*i] ) 
      {
      this->GraphBounds[2*i+1] = this->GraphBounds[2*i] + 1;
      }
    }
          
  // Generate the points, either x,y,0 or x,y,z
  vtkPoints* newPoints = vtkPoints::New();
  for (int i=0; i< graph->GetNumberOfVertices(); i++)
    {
    double x, y, z, r;
    r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    x = (this->GraphBounds[1] - this->GraphBounds[0])*r + this->GraphBounds[0];
    r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    y = (this->GraphBounds[3] - this->GraphBounds[2])*r + this->GraphBounds[2];
    if (this->ThreeDimensionalLayout)
      {
      r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
      z = (this->GraphBounds[5] - this->GraphBounds[4])*r + this->GraphBounds[4];
      }
    else
      {
      z = 0;
      }
    newPoints->InsertNextPoint(x, y, z);
    }

  // Set the graph points.
  graph->SetPoints(newPoints);

  // Clean up.
  newPoints->Delete();
}

void vtkRandomLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutomaticBoundsComputation: " 
     << (this->AutomaticBoundsComputation ? "On\n" : "Off\n");

  os << indent << "GraphBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->GraphBounds[0] << ", " 
     << this->GraphBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->GraphBounds[2] << ", " 
     << this->GraphBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->GraphBounds[4] << ", " 
     << this->GraphBounds[5] << ")\n";

  os << indent << "Three Dimensional Layout: " 
     << (this->ThreeDimensionalLayout ? "On\n" : "Off\n");
}
