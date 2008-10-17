/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingDefaultLayoutStrategy.cxx

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
#include "vtkTreeRingDefaultLayoutStrategy.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"

#include "vtkTree.h"

vtkCxxRevisionMacro(vtkTreeRingDefaultLayoutStrategy, "1.3");
vtkStandardNewMacro(vtkTreeRingDefaultLayoutStrategy);

vtkTreeRingDefaultLayoutStrategy::vtkTreeRingDefaultLayoutStrategy()
{
  this->SizeFieldName = 0;
  this->SetSizeFieldName("size");
}

vtkTreeRingDefaultLayoutStrategy::~vtkTreeRingDefaultLayoutStrategy()
{
  this->SetSizeFieldName(0);
}

void vtkTreeRingDefaultLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SizeFieldName: " << (this->SizeFieldName ? this->SizeFieldName : "(none)") << endl;
}

void vtkTreeRingDefaultLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  // Get the size array
  vtkDataArray* sizeArray = 
      inputTree->GetVertexData()->GetArray(this->SizeFieldName);
  
  if( inputTree->GetNumberOfVertices() == 0 )
      return;

    // Get the root vertex and set it to 0,1,0,1
  vtkIdType rootId = inputTree->GetRoot();
  float coords[] = {0.,this->InteriorRadius,0.,360.};
  coordsArray->SetTuple(rootId, coords);
  
    // Now layout the children vertices
  this->LayoutChildren(inputTree, coordsArray, sizeArray, inputTree->GetNumberOfChildren(rootId),
                       rootId, 0, coords[1], coords[2], coords[3]);

  vtkPoints* points = vtkPoints::New();
  vtkIdType numVerts = inputTree->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for( vtkIdType i = 0; i < numVerts; i++ )
  {
    if( i == 0 )
    {
      points->SetPoint( i, 0, 0, 0 );
      continue;
    }
    
    double sector_coords[4];
    coordsArray->GetTuple( i, sector_coords );
    double r = (0.5*(sector_coords[1] - sector_coords[0])) + sector_coords[0];
    double theta = sector_coords[2] + (0.5*(sector_coords[3]-sector_coords[2]));
    double x = r*cos(vtkMath::DegreesToRadians()*theta);
    double y = r*sin(vtkMath::DegreesToRadians()*theta);
    double z = 0.;
    points->SetPoint(i, x, y, z);
  }
  inputTree->SetPoints(points);
  points->Delete();
}

void vtkTreeRingDefaultLayoutStrategy::LayoutChildren(
    vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray, 
    vtkIdType nchildren, vtkIdType parent, vtkIdType begin, 
    float parentOuterRad, float parentStartAng, float parentEndAng)
{
  double new_interior_rad = parentOuterRad;
  double new_outer_rad = new_interior_rad + this->RingThickness;
//FIXME - we may want to do this instead...
    //double new_outer_rad = new_interior_rad +this->RingThickness[level];
  
    //now calculate the width of each of the sectors for each vertex
    // first calculate the total summed weight for each of the children vertices
  double total_weighted_sum = 0;
  vtkIdType i;
  for( i = begin; i < nchildren; i++)
  {
    total_weighted_sum += static_cast<float>(sizeArray->GetTuple1(tree->GetChild(parent, i)));
  }

  float coords[4];
  double current_angle = parentStartAng;
  double available_arc = parentEndAng - parentStartAng;
  for( i = begin; i < nchildren; i++)
  {
    int id = tree->GetChild(parent, i);
    double this_arc = available_arc * 
        ( static_cast<float>(sizeArray->GetTuple1(id)) / total_weighted_sum );

    coords[0] = new_interior_rad;
    coords[1] = new_outer_rad;
    coords[2] = current_angle;
    coords[3] = current_angle + this_arc;
    if( i == nchildren )
    {
      coords[3] = parentEndAng;
    }
    
    coordsArray->SetTuple(id, coords);

    current_angle += this_arc;

    vtkIdType numNewChildren = tree->GetNumberOfChildren(id);
    if (numNewChildren > 0)
    {
      this->LayoutChildren(tree, coordsArray, sizeArray, numNewChildren, id, 0,
                           coords[1], coords[2], coords[3]);
    }
  }
}
