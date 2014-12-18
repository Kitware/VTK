/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWeightEuclideanDistanceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraphWeightEuclideanDistanceFilter - Weights the edges of a
// graph based on the Euclidean distance between the points.
//
// .SECTION Description
// Weights the edges of a graph based on the Euclidean distance between the points.

#ifndef vtkGraphWeightEuclideanDistanceFilter_h
#define vtkGraphWeightEuclideanDistanceFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkGraphWeightFilter.h"

class vtkGraph;

class VTKFILTERSGENERAL_EXPORT vtkGraphWeightEuclideanDistanceFilter : public vtkGraphWeightFilter
{
 public:
  static vtkGraphWeightEuclideanDistanceFilter *New();
  vtkTypeMacro(vtkGraphWeightEuclideanDistanceFilter, vtkGraphWeightFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkGraphWeightEuclideanDistanceFilter(){}
  ~vtkGraphWeightEuclideanDistanceFilter(){}

  // Description:
  // Compute the Euclidean distance between the Points defined for the
  // verticies of a specified 'edge'.
  float ComputeWeight(vtkGraph* const graph, const vtkEdgeType& edge) const;

  // Description:
  // Ensure that 'graph' has Points defined.
  bool CheckRequirements(vtkGraph* const graph) const;

private:
  vtkGraphWeightEuclideanDistanceFilter(const vtkGraphWeightEuclideanDistanceFilter&);  // Not implemented.
  void operator=(const vtkGraphWeightEuclideanDistanceFilter&);  // Not implemented.
};

#endif
