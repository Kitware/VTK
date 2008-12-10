/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkTreeRingPointLayout.cxx

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

#include "vtkTreeRingPointLayout.h"

#include "vtkDataArray.h"
#include "vtkIntArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkTree.h"
#include "vtkTreeLevelsFilter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingPointLayout, "1.8");
vtkStandardNewMacro(vtkTreeRingPointLayout);

vtkTreeRingPointLayout::vtkTreeRingPointLayout()
{
  this->SectorsFieldName = 0;
  this->SetSectorsFieldName("sectors");
  this->ExteriorRadius = 1.;
  this->LogSpacingValue = 1.0;
}

vtkTreeRingPointLayout::~vtkTreeRingPointLayout()
{
  this->SetSectorsFieldName(0);
}

int vtkTreeRingPointLayout::RequestData( vtkInformation *vtkNotUsed(request),
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  if (this->SectorsFieldName == NULL)
    {
    vtkErrorMacro(<< "Sector field name must be non-null.");
    return 0;
    }
  
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // Storing the inputTree and outputTree handles
  vtkTree *inputTree = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *outputTree = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkDataArray* sectorsArray = inputTree->GetVertexData()->GetArray(this->SectorsFieldName);
  if( sectorsArray == NULL )
    {
    vtkErrorMacro(<< "SectorsArray must be non-NULL.");
    return 0;
    }
  
  VTK_CREATE(vtkTreeLevelsFilter, levelFilter);
  VTK_CREATE(vtkTree, newTree);
  newTree->ShallowCopy( inputTree );
  levelFilter->SetInput( newTree );
  levelFilter->Update();
  vtkTree* levelTree = levelFilter->GetOutput();
  outputTree->ShallowCopy( levelTree );
  
  vtkIntArray* levelArray = vtkIntArray::SafeDownCast( levelTree->GetVertexData()->GetAbstractArray("level") );
  vtkIntArray* leafArray = vtkIntArray::SafeDownCast( levelTree->GetVertexData()->GetAbstractArray("leaf") );
  
  int max_level = 0;
  for( int i = 0; i < outputTree->GetNumberOfVertices(); i++ )
    {
    int level = levelArray->GetValue(i);
    if( level > max_level )
      {
      max_level = level;
      }
    }
  
  double spacing = this->LogSpacingValue;
  
  // The distance between level L-1 and L is s^L.
  // Thus, if s < 1 then the distance between levels gets smaller in higher levels,
  //       if s = 1 the distance remains the same, and
  //       if s > 1 the distance get larger.
  // The height (distance from the root) of level L, then, is
  // s + s^2 + s^3 + ... + s^L, where s is the log spacing value.
  // The max height (used for normalization) is
  // s + s^2 + s^3 + ... + s^maxLevel.
  // The quick formula for computing this is
  // sum_{i=1}^{n} s^i = (s^(n+1) - 1)/(s - 1) - 1        if s != 1
  //                   = n                                if s == 1
  double maxHeight = max_level;
  double eps = 1e-8;
  double diff = spacing - 1.0 > 0 ? spacing - 1.0 : 1.0 - spacing;
  if (diff > eps)
    {
    maxHeight = (pow(spacing, max_level+1.0) - 1.0)/(spacing - 1.0) - 1.0;
    }
  
  vtkPoints* points = vtkPoints::New();
  vtkIdType rootId = outputTree->GetRoot();
  vtkIdType numVerts = outputTree->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for( vtkIdType i = 0; i < numVerts; i++ )
    {
    if( i == rootId )
      {
      points->SetPoint( i, 0, 0, 0 );
      continue;
      }
    
    double sector_coords[4];
    sectorsArray->GetTuple( i, sector_coords );
    
    double r;
    if( leafArray->GetValue(i) == 1 )
      {
      r = sector_coords[2];
      }
    else
      {
      if (diff <= eps)
        {
        r = outputTree->GetLevel(i)/maxHeight;
        }
      else
        {
        r = ((pow(spacing, outputTree->GetLevel(i)+1.0) - 1.0)/(spacing - 1.0) - 1.0)/maxHeight;
        } 
      //scale the spacing value based on the radius of the
      // circle we have to work with...
      r *= this->ExteriorRadius;
      }
    
    double theta = sector_coords[0] + (0.5*(sector_coords[1]-sector_coords[0]));
    double x = r * cos( vtkMath::RadiansFromDegrees( theta ) );
    double y = r * sin( vtkMath::RadiansFromDegrees( theta ) );
    double z = 0.;
    points->SetPoint(i, x, y, z);
    }
  outputTree->SetPoints(points);
  points->Delete();
  
  return 1;
}

void vtkTreeRingPointLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SectorsFieldName: " << (this->SectorsFieldName ? this->SectorsFieldName : "(none)") << endl;
  os << indent << "LogSpacingValue: " << this->LogSpacingValue << endl;
  os << indent << "ExteriorRadius: " << this->ExteriorRadius << endl;
}
