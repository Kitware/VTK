/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingExistingLayoutStrategy.cxx

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
#include "vtkTreeRingExistingLayoutStrategy.h"

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
#include "vtkTreeLevelsFilter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingExistingLayoutStrategy, "1.1");
vtkStandardNewMacro(vtkTreeRingExistingLayoutStrategy);

vtkTreeRingExistingLayoutStrategy::vtkTreeRingExistingLayoutStrategy()
{
  this->SizeFieldName = 0;
  this->SetSizeFieldName("size");
}

vtkTreeRingExistingLayoutStrategy::~vtkTreeRingExistingLayoutStrategy()
{
  this->SetSizeFieldName(0);
}

void vtkTreeRingExistingLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SizeFieldName: " << (this->SizeFieldName ? this->SizeFieldName : "(none)") << endl;
}

void vtkTreeRingExistingLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  // Get the size array
  vtkDataArray* sizeArray = 
      inputTree->GetVertexData()->GetArray(this->SizeFieldName);
  
  if( inputTree->GetNumberOfVertices() == 0 )
      return;

  VTK_CREATE(vtkTreeLevelsFilter, levelFilter);
  levelFilter->SetInput( inputTree );
  levelFilter->Update();
  vtkTree* levelTree = levelFilter->GetOutput();
  
  vtkDataArray *levelArray = levelTree->GetVertexData()->GetArray("level");
  int max_level = 0;
  for( int i = 0; i < levelTree->GetNumberOfVertices(); i++ )
  {
    int level = levelArray->GetTuple1(i);
    if( level > max_level )
    {
      max_level = level;
    }
  }

  vtkDataArray *anglesArray = levelTree->GetVertexData()->GetArray("subtended_angles");
  if( anglesArray == NULL )
  {
    vtkErrorMacro("Could not find subtended_angles array.");
    return;
  }
        
    
  double outer_radius = max_level*this->RingThickness + this->InteriorRadius;
  
      // Get the root vertex and set it to 0,1,0,1
  vtkIdType rootId = levelTree->GetRoot();
  float coords[] = {outer_radius-this->RingThickness, outer_radius, 0., 360.};
  coordsArray->SetTuple(rootId, coords);

    // Now layout the children vertices
  this->LayoutChildren(levelTree, coordsArray, sizeArray, anglesArray, levelTree->GetNumberOfChildren(rootId),
                       rootId, 0, coords[0], coords[1], coords[2], coords[3]);
}

void vtkTreeRingExistingLayoutStrategy::LayoutChildren(
    vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray, vtkDataArray *anglesArray, 
    vtkIdType nchildren, vtkIdType parent, vtkIdType begin, 
    float parentInnerRad, float parentOuterRad, float parentStartAng, float parentEndAng)
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
  for (vtkIdType i = begin; i < nchildren; i++)
  {
    int id = tree->GetChild(parent, i);
    double this_arc = available_arc * 
        ( static_cast<float>(sizeArray->GetTuple1(id)) / total_weighted_sum );

    double angles[2];
    anglesArray->GetTuple( id, angles );

    coords[0] = new_interior_rad;
    coords[1] = new_outer_rad;
    coords[2] = angles[0];
    coords[3] = angles[1];
//    coords[2] = current_angle;
//    coords[3] = current_angle + this_arc;
    if( i == nchildren )
    {
      coords[3] = parentEndAng;
    }
    
    coordsArray->SetTuple(id, coords);

    current_angle += this_arc;

    vtkIdType numNewChildren = tree->GetNumberOfChildren(id);
    if (numNewChildren > 0)
    {
      this->LayoutChildren(tree, coordsArray, sizeArray, anglesArray, numNewChildren, id, 0,
                           coords[0], coords[1], coords[2], coords[3]);
    }
  }
}
