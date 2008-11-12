/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingReversedLayoutStrategy.cxx

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
#include "vtkTreeRingReversedLayoutStrategy.h"

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
#include "vtkIntArray.h"
#include "vtkTree.h"
#include "vtkTreeLevelsFilter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingReversedLayoutStrategy, "1.8");
vtkStandardNewMacro(vtkTreeRingReversedLayoutStrategy);

vtkTreeRingReversedLayoutStrategy::vtkTreeRingReversedLayoutStrategy()
{
  this->SizeFieldName = 0;
  this->SetSizeFieldName("size");
}

vtkTreeRingReversedLayoutStrategy::~vtkTreeRingReversedLayoutStrategy()
{
  this->SetSizeFieldName(0);
}

void vtkTreeRingReversedLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SizeFieldName: " << (this->SizeFieldName ? this->SizeFieldName : "(none)") << endl;
}

void vtkTreeRingReversedLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  // Get the size array
  vtkDataArray* sizeArray = 
      inputTree->GetVertexData()->GetArray(this->SizeFieldName);
  
  if( inputTree->GetNumberOfVertices() == 0 )
      return;

  VTK_CREATE(vtkTreeLevelsFilter, levelFilter);
  VTK_CREATE(vtkTree, newTree);
  newTree->ShallowCopy( inputTree );
  levelFilter->SetInput( newTree );
  levelFilter->Update();
  vtkTree* levelTree = levelFilter->GetOutput();
  
  vtkIntArray *levelArray = vtkIntArray::SafeDownCast( levelTree->GetVertexData()->GetAbstractArray("level") );
  int max_level = 0;
  for( int i = 0; i < levelTree->GetNumberOfVertices(); i++ )
  {
    int level = levelArray->GetValue(i);
    if( level > max_level )
    {
      max_level = level;
    }
  }

  double outer_radius = max_level*this->RingThickness + this->InteriorRadius;
  
      // Get the root vertex and set it to 0,1,0,1
  vtkIdType rootId = levelTree->GetRoot();
  float coords[] = {this->RootStartAngle, this->RootEndAngle, outer_radius-this->RingThickness, outer_radius};
  coordsArray->SetTuple(rootId, coords);

    // Now layout the children vertices
  this->LayoutChildren(levelTree, coordsArray, sizeArray, 
                       levelTree->GetNumberOfChildren(rootId),
                       rootId, 0, coords[2], coords[0], coords[1]);

  vtkPoints* points = vtkPoints::New();
  vtkIdType numVerts = inputTree->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for( vtkIdType i = 0; i < numVerts; i++ )
  {
    double sector_coords[4];
    coordsArray->GetTuple( i, sector_coords );
    double x, y, z;
    if( this->UseRectangularCoordinates )
    {
      x = 0.5*(sector_coords[0] + sector_coords[1]);
      y = 0.5*(sector_coords[2] + sector_coords[3]);
      z = 0.;
    }
    else
    {
      if( i == rootId )
      {
        x = y = z = 0.;
      }
      else
      {
        double r = (0.5*(sector_coords[3] - sector_coords[2])) + sector_coords[2];
        double theta = sector_coords[0] + (0.5*(sector_coords[1]-sector_coords[0]));
        x = r * cos( vtkMath::RadiansFromDegrees( theta ) );
        y = r * sin( vtkMath::RadiansFromDegrees( theta ) );
        z = 0.;
      }
    }
    points->SetPoint(i, x, y, z);
  }
  inputTree->SetPoints(points);
  points->Delete();
}

void vtkTreeRingReversedLayoutStrategy::LayoutChildren(
    vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray, 
    vtkIdType nchildren, vtkIdType parent, vtkIdType begin, 
    float parentInnerRad, float parentStartAng, float parentEndAng)
{
  double new_interior_rad = parentInnerRad - this->RingThickness;
  double new_outer_rad = parentInnerRad;
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

    coords[2] = new_interior_rad;
    coords[3] = new_outer_rad;
    coords[0] = current_angle;
    coords[1] = current_angle + this_arc;
    if( i == nchildren )
    {
      coords[1] = parentEndAng;
    }
    
    coordsArray->SetTuple(id, coords);

    current_angle += this_arc;

    vtkIdType numNewChildren = tree->GetNumberOfChildren(id);
    if (numNewChildren > 0)
    {
      this->LayoutChildren(tree, coordsArray, sizeArray, numNewChildren, id, 0,
                           coords[2], coords[0], coords[1]);
    }
  }
}
