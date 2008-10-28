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
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingPointLayout, "1.1");
vtkStandardNewMacro(vtkTreeRingPointLayout);

vtkTreeRingPointLayout::vtkTreeRingPointLayout()
{
  this->SectorsFieldName = 0;
  this->SetSectorsFieldName("sectors");
  this->ExteriorRadius = 1.;
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

  double spacing = this->ExteriorRadius / (double)max_level;
  
  vtkPoints* points = vtkPoints::New();
  vtkIdType numVerts = outputTree->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for( vtkIdType i = 0; i < numVerts; i++ )
  {
    if( i == 0 )
    {
      points->SetPoint( i, 0, 0, 0 );
      continue;
    }
  
    double sector_coords[4];
    sectorsArray->GetTuple( i, sector_coords );

    double r;
    if( leafArray->GetValue(i) == 1 )
    {
      r = sector_coords[0];
    }
    else
    {
      int current_level = levelArray->GetValue(i);
      r = spacing*current_level;
    }
    
      //double r = (0.5*(sector_coords[1] - sector_coords[0])) + sector_coords[0];
    double theta = sector_coords[2] + (0.5*(sector_coords[3]-sector_coords[2]));
    double x = r*cos(vtkMath::DegreesToRadians()*theta);
    double y = r*sin(vtkMath::DegreesToRadians()*theta);
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
}
