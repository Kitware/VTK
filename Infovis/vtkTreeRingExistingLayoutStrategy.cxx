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

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkTree.h"
#include "vtkTreeLevelsFilter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingExistingLayoutStrategy, "1.3");
vtkStandardNewMacro(vtkTreeRingExistingLayoutStrategy);

vtkTreeRingExistingLayoutStrategy::vtkTreeRingExistingLayoutStrategy()
{
//   this->SizeFieldName = 0;
//   this->SetSizeFieldName("size");
}

vtkTreeRingExistingLayoutStrategy::~vtkTreeRingExistingLayoutStrategy()
{
//  this->SetSizeFieldName(0);
}

void vtkTreeRingExistingLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
//  os << indent << "SizeFieldName: " << (this->SizeFieldName ? this->SizeFieldName : "(none)") << endl;
}

void vtkTreeRingExistingLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  // Get the size array
//   vtkDataArray* sizeArray = 
//       inputTree->GetVertexData()->GetArray(this->SizeFieldName);
  
  if( inputTree->GetNumberOfVertices() == 0 )
      return;

  VTK_CREATE(vtkTreeLevelsFilter, levelFilter);
//  levelFilter->SetInput( inputTree );
  VTK_CREATE(vtkTree, newTree);
  newTree->ShallowCopy( inputTree );
  levelFilter->SetInput( newTree );
  levelFilter->Update();
  vtkTree* levelTree = levelFilter->GetOutput();
  
  vtkIntArray *levelArray = vtkIntArray::SafeDownCast( levelTree->GetVertexData()->GetAbstractArray("level") );
  if( levelArray == NULL )
  {
    vtkErrorMacro( "Unable to get an array for tree levels." );
    return;
  }
  
  int i, max_level = 0;
  for( i = 0; i < levelTree->GetNumberOfVertices(); i++ )
  {
    int level = levelArray->GetValue(i);
    if( level > max_level )
    {
      max_level = level;
    }
  }

    //check for a flat tree...
  double x[3];
  bool point_set = false;
  vtkDataArray *leafArray = levelTree->GetVertexData()->GetArray("leaf");
  for( i = 0; i < levelTree->GetNumberOfVertices(); i++ )
  {
    int level = levelArray->GetTuple1(i);
    int leaf = leafArray->GetTuple1(i);
    if( leaf )
    {
      if( level != max_level )
      {
        vtkErrorMacro( "Tree is not flat." );
        return;
      }
      
      if( point_set == false )
      {
//        inputTree->GetPoint( i, x );
        levelTree->GetPoint( i, x );
        point_set = true;
      }
    }
  }
    
  if( point_set == false )
  {
    vtkErrorMacro("Unable to calculate interior radius.");
    return;
  }

  double r = sqrt( x[0]*x[0] + x[1]*x[1] );
  this->SetInteriorRadius( r );
  
  vtkDataArray *anglesArray = levelTree->GetVertexData()->GetArray("subtended_angles");
  if( anglesArray == NULL )
  {
    vtkErrorMacro("Could not find subtended_angles array.");
    return;
  }

  double outer_radius = ((max_level+1)*this->RingThickness) + this->InteriorRadius;  
  vtkIdType rootId = levelTree->GetRoot();
  float coords[] = {outer_radius-this->RingThickness, outer_radius, 0., 360.};
  coordsArray->SetTuple(rootId, coords);

//FIXME-jfsheph - Need to calculate parent angles by summing up the subtended angles of the children...
  double min_angle, max_angle;
  this->SetInteriorSubtendedAngles(levelTree, rootId, anglesArray, min_angle, max_angle);
  
    // Now layout the children vertices
  this->LayoutChildren(levelTree, coordsArray, anglesArray, 
                       levelTree->GetNumberOfChildren(rootId),
                       rootId, coords[0]);

  vtkPoints* points = vtkPoints::New();
//  vtkIdType numVerts = inputTree->GetNumberOfVertices();
  vtkIdType numVerts = levelTree->GetNumberOfVertices();
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
//  levelTree->SetPoints(points);
  points->Delete();
}

// void vtkTreeRingExistingLayoutStrategy::LayoutChildren(
//     vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray, vtkDataArray *anglesArray, 
//     vtkIdType nchildren, vtkIdType parent, vtkIdType begin, 
//     float parentInnerRad, float parentStartAng, float parentEndAng)
void vtkTreeRingExistingLayoutStrategy::LayoutChildren(
    vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *anglesArray, 
    vtkIdType nchildren, vtkIdType parent, float parentInnerRad )
{
  double new_interior_rad = parentInnerRad - this->RingThickness;
  double new_outer_rad = parentInnerRad;
//FIXME - we may want to do this instead...
    //double new_outer_rad = new_interior_rad +this->RingThickness[level];
  
    //now calculate the width of each of the sectors for each vertex
    // first calculate the total summed weight for each of the children vertices
  vtkIdType i;
//   double total_weighted_sum = 0;
//   for( i = begin; i < nchildren; i++)
//   {
//     total_weighted_sum += static_cast<float>(sizeArray->GetTuple1(tree->GetChild(parent, i)));
//   }

  float coords[4];
//   double current_angle = parentStartAng;
//   double available_arc = parentEndAng - parentStartAng;
  for( i = 0; i < nchildren; i++)
  {
    int id = tree->GetChild(parent, i);
//     double this_arc = available_arc * 
//         ( static_cast<float>(sizeArray->GetTuple1(id)) / total_weighted_sum );

    double angles[2];
    anglesArray->GetTuple( id, angles );

    coords[0] = new_interior_rad;
    coords[1] = new_outer_rad;
    coords[2] = angles[0];
    coords[3] = angles[1];
//    coords[2] = current_angle;
//    coords[3] = current_angle + this_arc;
//     if( i == nchildren )
//     {
//       coords[3] = parentEndAng;
//     }
    
    coordsArray->SetTuple(id, coords);

//    current_angle += this_arc;

    vtkIdType numNewChildren = tree->GetNumberOfChildren(id);
    if (numNewChildren > 0)
    {
      this->LayoutChildren(tree, coordsArray, anglesArray, numNewChildren, id, coords[0]);
    }
  }
}

void vtkTreeRingExistingLayoutStrategy::SetInteriorSubtendedAngles(
    vtkTree* tree, vtkIdType parent, vtkDataArray *anglesArray, 
    double& min_angle, double& max_angle)
{
  int num_children = tree->GetNumberOfChildren(parent);
  double current_angles[2];
  anglesArray->GetTuple( parent, current_angles );

  if( num_children > 0 )
  {
    vtkIdType i;
    for( i = 0; i < num_children; i++ )
    {
      int id = tree->GetChild(parent, i);
      
      double child_angles[2];    
      this->SetInteriorSubtendedAngles(tree, id, anglesArray, child_angles[0], child_angles[1]);
      
      if( child_angles[0] < current_angles[0] )
      {
        current_angles[0] = child_angles[0];
      }
      
      if( child_angles[1] > current_angles[1] )
      {
        current_angles[1] = child_angles[1];
      }
    }
    
    anglesArray->SetTuple( parent, current_angles );
  }

  min_angle = current_angles[0];
  max_angle = current_angles[1];
}

