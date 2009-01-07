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

vtkCxxRevisionMacro(vtkPerturbCoincidentVertices, "1.1");
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

  output->ShallowCopy(input);
  output->GetFieldData()->PassData(input->GetFieldData());
  
  vtkPoints* points = output->GetPoints();
  int numPoints = points->GetNumberOfPoints();

  double bounds[6]; // xmin, xmax, ymin, ymax, zmin, zmax
  points->ComputeBounds();
  points->GetBounds(bounds);
  double point1[3] = { bounds[0], bounds[2], bounds[4] };
  double point2[3] = { bounds[1], bounds[3], bounds[5] };

  for(int i = 0; i < numPoints; ++i)
    {
    this->CoincidentPoints->AddPoint(i, points->GetPoint(i));
    }

  this->CoincidentPoints->RemoveNonCoincidentPoints();
  this->CoincidentPoints->InitTraversal();

  double spiralPoint[3];
  double point[3];
  double scale = sqrt(vtkMath::Distance2BetweenPoints(point1, point2))/points->GetNumberOfPoints();
  vtkSmartPointer<vtkPoints> offsets = vtkSmartPointer<vtkPoints>::New();
  int numCoincidentPoints = 0;
  vtkIdList * coincidentPoints = this->CoincidentPoints->GetNextCoincidentPointIds();
  vtkIdType Id = 0;
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
