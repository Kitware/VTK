/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerturbCoincidentVertices.cxx

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

#include "vtkPerturbCoincidentVertices.h"

#include "vtkCoincidentPoints.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkPerturbCoincidentVertices, "1.4");
vtkStandardNewMacro(vtkPerturbCoincidentVertices);
//----------------------------------------------------------------------------
vtkPerturbCoincidentVertices::vtkPerturbCoincidentVertices()
{
  this->CoincidentPoints = vtkSmartPointer<vtkCoincidentPoints>::New();
}

//----------------------------------------------------------------------------
vtkPerturbCoincidentVertices::~vtkPerturbCoincidentVertices()
{
}

//----------------------------------------------------------------------------
int vtkPerturbCoincidentVertices::RequestData(
  vtkInformation* vtkNotUsed(request), 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkGraph* input = vtkGraph::GetData(inputVector[0]);
  vtkGraph* output = vtkGraph::GetData(outputVector);

  output->DeepCopy(input);
  output->GetFieldData()->DeepCopy(input->GetFieldData());
  
  vtkPoints* points = output->GetPoints();
  int numPoints = points->GetNumberOfPoints();

  double bounds[6]; // xmin, xmax, ymin, ymax, zmin, zmax
  points->ComputeBounds();
  points->GetBounds(bounds);
  double point1[3] = { bounds[0], bounds[2], bounds[4] };
  double point2[3] = { bounds[1], bounds[3], bounds[5] };
  double vertEdge1[3], vertEdge2[3], boundingDims[3];
  int numCoincidentPoints = 0, i = 0, j = 0;

  for(i = 0; i < numPoints; ++i)
    {
    this->CoincidentPoints->AddPoint(i, points->GetPoint(i));
    }

  this->CoincidentPoints->RemoveNonCoincidentPoints();
  this->CoincidentPoints->InitTraversal();

  vtkIdType Id = 0;
  vtkIdType vertId = 0;
  vtkIdType vertInDegree = 0;
  vtkIdType vertOutDegree = 0;
  double edgeLength = VTK_DOUBLE_MAX;
  double shortestEdge = VTK_DOUBLE_MAX;
  vtkInEdgeType inEdge;
  vtkOutEdgeType outEdge;

  // Here we compute 2 metrics, the length of the shortest edge connected to any coincident point or
  // the average point distance assuming the points are uniformly distributed. The smallest of these
  // two metrics will be used to scale the spiral.

  // Compute shortest edge comming to/from the coincident points.
  vtkIdList * coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
  while(coincidentPoints != NULL)
    {
    numCoincidentPoints = coincidentPoints->GetNumberOfIds();
    for(i = 0; i < numCoincidentPoints; ++i)
      {
      vertId = coincidentPoints->GetId(i);
      vertInDegree = input->GetInDegree(vertId);
      vertOutDegree = input->GetOutDegree(vertId);
      points->GetPoint(vertId, vertEdge1);

      for(j = 0; j < vertInDegree; ++j)
        {
        inEdge = input->GetInEdge(vertId, j);
        points->GetPoint(inEdge.Source, vertEdge2);

        if(vertEdge1[0] != vertEdge2[0] ||
           vertEdge1[1] != vertEdge2[1] ||
           vertEdge1[2] != vertEdge2[2])
          {
          edgeLength = vtkMath::Distance2BetweenPoints(vertEdge1, vertEdge2);
          }
        shortestEdge = edgeLength < shortestEdge ? edgeLength : shortestEdge;
        }
      for(j = 0; j < vertOutDegree; ++j)
        {
        outEdge = input->GetOutEdge(vertId, j);
        points->GetPoint(outEdge.Target, vertEdge2);
        if(vertEdge1[0] != vertEdge2[0] ||
           vertEdge1[1] != vertEdge2[1] ||
           vertEdge1[2] != vertEdge2[2])
          {
          edgeLength = vtkMath::Distance2BetweenPoints(vertEdge1, vertEdge2);
          }
        shortestEdge = edgeLength < shortestEdge ? edgeLength : shortestEdge;
        }
      }
    coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
    }
  shortestEdge = sqrt(shortestEdge);

  // Compute the average distance assuming all the points are uniformly dispersed
  // through the bounding box.
  boundingDims[0] = bounds[1] - bounds[0];
  boundingDims[1] = bounds[3] - bounds[2];
  boundingDims[2] = bounds[5] - bounds[4];

  double averageDistance = 0.0;

  if(boundingDims[2] == 0.0)
    {
    averageDistance = sqrt((boundingDims[0]*boundingDims[1])/numPoints);
    }
  else
    {
    averageDistance = pow(
      (boundingDims[0]*boundingDims[1]*boundingDims[2])/static_cast<double>(numPoints), 1.0/3.0);
    }

  double spiralPoint[3];
  double point[3];
  //double distance = sqrt(vtkMath::Distance2BetweenPoints(point1, point2));
  double scale = 1.0;
  //scale = (distance * 1.4)/numPoints;

  // use the smallest metric to scale the spiral vertices.
  scale = shortestEdge < averageDistance ? shortestEdge/4 : averageDistance/4;
  vtkSmartPointer<vtkPoints> offsets = vtkSmartPointer<vtkPoints>::New();
  
  this->CoincidentPoints->InitTraversal();
  coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
  // Iterate over each coordinate that may have a set of coincident point ids.
  while(coincidentPoints != NULL)
    {
    // Iterate over all coincident point ids and perturb them
    numCoincidentPoints = coincidentPoints->GetNumberOfIds();
    vtkMath::SpiralPoints( numCoincidentPoints + 1, offsets );
    for(int i = 0; i < numCoincidentPoints; ++i)
      {
      Id = coincidentPoints->GetId(i);
      points->GetPoint(Id, point);
      offsets->GetPoint(i + 1, spiralPoint);

      points->SetPoint(Id,
        point[0] + spiralPoint[0] * scale,
        point[1] + spiralPoint[1] * scale,
        point[2] );
      }
    coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPerturbCoincidentVertices::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
