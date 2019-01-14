/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWeightEuclideanDistanceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphWeightEuclideanDistanceFilter.h"

#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkGraphWeightEuclideanDistanceFilter);

float vtkGraphWeightEuclideanDistanceFilter::ComputeWeight(vtkGraph* const graph,
                                                           const vtkEdgeType& edge) const
{
  double p1[3];
  graph->GetPoint(edge.Source, p1);

  double p2[3];
  graph->GetPoint(edge.Target, p2);

  float w = static_cast<float>(sqrt(vtkMath::Distance2BetweenPoints(p1, p2)));

  return w;
}

bool vtkGraphWeightEuclideanDistanceFilter::CheckRequirements(vtkGraph* const graph) const
{
  vtkPoints* points = graph->GetPoints();
  if(!points)
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkGraphWeightEuclideanDistanceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGraphWeightFilter::PrintSelf(os,indent);
}
