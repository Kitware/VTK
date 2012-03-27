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
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkPerturbCoincidentVertices.h"

#include "vtkBitArray.h"
#include "vtkCoincidentPoints.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <vector>

vtkStandardNewMacro(vtkPerturbCoincidentVertices);
//----------------------------------------------------------------------------
vtkPerturbCoincidentVertices::vtkPerturbCoincidentVertices()
{
  PerturbFactor = 1.0;
}

//----------------------------------------------------------------------------
vtkPerturbCoincidentVertices::~vtkPerturbCoincidentVertices()
{
}

//----------------------------------------------------------------------------
void vtkPerturbCoincidentVertices::SpiralPerturbation(vtkGraph *input, vtkGraph *output)
{

  // The points will be deep copied because they
  // will be modified (perturbed).
  output->ShallowCopy(input);
  output->GetPoints()->DeepCopy(input->GetPoints());
  vtkPoints* points = output->GetPoints();

  int numPoints = points->GetNumberOfPoints();
  double bounds[6]; // xmin, xmax, ymin, ymax, zmin, zmax
  points->ComputeBounds();
  points->GetBounds(bounds);
  //double point1[3] = { bounds[0], bounds[2], bounds[4] };
  //double point2[3] = { bounds[1], bounds[3], bounds[5] };
  double vertEdge1[3], vertEdge2[3], boundingDims[3];
  int numCoincidentPoints = 0, i = 0, j = 0;

  vtkSmartPointer<vtkCoincidentPoints> coincidentPoints =
    vtkSmartPointer<vtkCoincidentPoints>::New();
  for(i = 0; i < numPoints; ++i)
    {
    coincidentPoints->AddPoint(i, points->GetPoint(i));
    }

  coincidentPoints->RemoveNonCoincidentPoints();
  coincidentPoints->InitTraversal();

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

  // Compute shortest edge coming to/from the coincident points.
  vtkIdList * coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
  while(coincidentPointsList != NULL)
    {
    numCoincidentPoints = coincidentPointsList->GetNumberOfIds();
    for(i = 0; i < numCoincidentPoints; ++i)
      {
      vertId = coincidentPointsList->GetId(i);
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
    coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
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

  coincidentPoints->InitTraversal();
  coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
  // Iterate over each coordinate that may have a set of coincident point ids.
  while(coincidentPointsList != NULL)
    {
    // Iterate over all coincident point ids and perturb them
    numCoincidentPoints = coincidentPointsList->GetNumberOfIds();
    vtkCoincidentPoints::SpiralPoints( numCoincidentPoints + 1, offsets );
    for(i = 0; i < numCoincidentPoints; ++i)
      {
      Id = coincidentPointsList->GetId(i);
      points->GetPoint(Id, point);
      offsets->GetPoint(i + 1, spiralPoint);

      points->SetPoint(Id,
        point[0] + spiralPoint[0] * scale,
        point[1] + spiralPoint[1] * scale,
        point[2] );
      }
    coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
    }
}


// Temp datastructure
struct Coord
{
  double coord[2];
  Coord()
    {
    }
  Coord( const double src[3] )
    {
    this->coord[0] = src[0];
    this->coord[1] = src[1];
    }
  ~Coord() {}

   static double distance(Coord x,Coord y)
     {
      return ( ( x.coord[0] - y.coord[0] ) * ( x.coord[0] - y.coord[0] )
           + ( x.coord[1] - y.coord[1] ) * ( x.coord[1] - y.coord[1] ) );
     }
};

//----------------------------------------------------------------------------
void vtkPerturbCoincidentVertices::SimpleSpiralPerturbation(vtkGraph *input,
                                                          vtkGraph *output,
                                                          float perturbFactor)
{
  // The points will be deep copied because they
  // will be modified (perturbed).
  output->ShallowCopy(input);
  output->GetPoints()->DeepCopy(input->GetPoints());
  vtkPoints* points = output->GetPoints();


  int numPoints = points->GetNumberOfPoints();

  // Temporary abort as this perturbation method
  // calculates N^2 distances which doesn't scale well.
  if(numPoints > 1000)
    {
    return;
    }

  int numCoincidentPoints = 0;
  double spiralOffsets[3];
  double currentPoint[3];
  vtkIdType index;

  vtkSmartPointer<vtkTimerLog> timer =
    vtkSmartPointer<vtkTimerLog>::New();

  // Collect the coincident points into a nice list
  vtkSmartPointer<vtkCoincidentPoints> coincidentPoints =
    vtkSmartPointer<vtkCoincidentPoints>::New();
  for(int i = 0; i < numPoints; ++i)
    {
    coincidentPoints->AddPoint(i, points->GetPoint(i));
    }

  // Note: We're not going to remove the non-coincident
  // points until after computing the distance from all
  // the points that have distinct coordinates.
  coincidentPoints->InitTraversal();

  std::vector<Coord> coincidentFoci;
  vtkIdList * coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
  while(coincidentPointsList != NULL)
    {
    // Just grabbing the first vertex of each coincident foci
    vtkIdType vertexIndex = coincidentPointsList->GetId(0);
    points->GetPoint(vertexIndex,currentPoint);
    coincidentFoci.push_back(currentPoint);

    // Get next coincident point list
    coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
    }

  // Compute the shortest intra-distance between coincident point foci
  double shortestDistance = VTK_DOUBLE_MAX;
  int numberOfFoci = static_cast<int>(coincidentFoci.size());
  if (numberOfFoci > 1)
    {
    for (int i=0; i<numberOfFoci; ++i)
      {
      for (int j=i+1; j<numberOfFoci; ++j)
        {
        double distance = Coord::distance(coincidentFoci[i], coincidentFoci[j]);
        shortestDistance = distance < shortestDistance ? distance : shortestDistance;
        }
      }
    }
  else
    {
    shortestDistance = 0.;
    }

  // Set the offset distance to be the shortest distance /4 * user setting (perturbFactor)
  double offsetDistance = sqrt(shortestDistance)/4.0 * perturbFactor;

  // These store the offsets for a spiral with a certain number of points
  vtkSmartPointer<vtkPoints> offsets = vtkSmartPointer<vtkPoints>::New();

  // Removing the coincident points and re-initializing the iterator.
  coincidentPoints->RemoveNonCoincidentPoints();
  coincidentPoints->InitTraversal();
  coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();

  // Iterate over each coordinate that may have a set of coincident point ids.
  while(coincidentPointsList != NULL)
    {
    // Iterate over all coincident point ids and perturb them
    numCoincidentPoints = coincidentPointsList->GetNumberOfIds();
    vtkCoincidentPoints::SpiralPoints( numCoincidentPoints + 1, offsets );
    for(int i = 0; i < numCoincidentPoints; ++i)
      {
      index = coincidentPointsList->GetId(i);
      points->GetPoint(index, currentPoint);
      offsets->GetPoint(i + 1, spiralOffsets);

      points->SetPoint(index,
        currentPoint[0] + spiralOffsets[0] * offsetDistance,
        currentPoint[1] + spiralOffsets[1] * offsetDistance,
        currentPoint[2] );
      }
    coincidentPointsList = coincidentPoints->GetNextCoincidentPointIds();
    }
}

//----------------------------------------------------------------------------
int vtkPerturbCoincidentVertices::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkGraph* input = vtkGraph::GetData(inputVector[0]);
  vtkGraph* output = vtkGraph::GetData(outputVector);

  this->SimpleSpiralPerturbation(input, output, 1.0);

  return 1;
}



//----------------------------------------------------------------------------
void vtkPerturbCoincidentVertices::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PerturbFactor: " << this->PerturbFactor << "\n";
}
