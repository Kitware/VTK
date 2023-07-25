// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGraphWeightEuclideanDistanceFilter.h"

#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGraphWeightEuclideanDistanceFilter);

float vtkGraphWeightEuclideanDistanceFilter::ComputeWeight(
  vtkGraph* graph, const vtkEdgeType& edge) const
{
  double p1[3];
  graph->GetPoint(edge.Source, p1);

  double p2[3];
  graph->GetPoint(edge.Target, p2);

  float w = static_cast<float>(sqrt(vtkMath::Distance2BetweenPoints(p1, p2)));

  return w;
}

bool vtkGraphWeightEuclideanDistanceFilter::CheckRequirements(vtkGraph* graph) const
{
  vtkPoints* points = graph->GetPoints();
  if (!points)
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkGraphWeightEuclideanDistanceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGraphWeightFilter::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
