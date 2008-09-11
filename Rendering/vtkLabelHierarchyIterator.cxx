/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelHierarchyIterator.cxx

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

#include "vtkLabelHierarchyIterator.h"

#include "vtkCamera.h"
#include "vtkCellType.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkIntArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro( vtkLabelHierarchyIterator, "1.1" );
vtkCxxSetObjectMacro( vtkLabelHierarchyIterator, Hierarchy, vtkLabelHierarchy );

vtkLabelHierarchyIterator::vtkLabelHierarchyIterator()
{
  this->Hierarchy = 0;
}

vtkLabelHierarchyIterator::~vtkLabelHierarchyIterator()
{
  if (this->Hierarchy)
    {
    this->Hierarchy->Delete();
    }
}

void vtkLabelHierarchyIterator::GetPoint( double x[3] )
{
  this->Hierarchy->GetPoints()->GetPoint( this->GetLabelId(), x );
}

void vtkLabelHierarchyIterator::GetSize( double sz[2] )
{
  vtkDataArray* labelSizeArr = this->Hierarchy->GetPointData()->GetArray( "LabelSize" );
  if ( ! labelSizeArr )
    {
    sz[0] = sz[1] = 0.;
    return;
    }
  vtkIdType lid = this->GetLabelId();
  double* ls = labelSizeArr->GetTuple( lid );
  sz[0] = ls[0];
  sz[1] = ls[1];
}

int vtkLabelHierarchyIterator::GetType()
{
  vtkDataArray* labelTypeArr = this->Hierarchy->GetPointData()->GetArray( "Type" );
  if ( ! labelTypeArr )
    {
    return -1;
    }
  vtkIntArray* labelTypeIArr = vtkIntArray::SafeDownCast( labelTypeArr );
  if ( ! labelTypeIArr )
    {
    return -1;
    }
  vtkIdType lid = this->GetLabelId();
  return labelTypeIArr->GetValue( lid );
}

