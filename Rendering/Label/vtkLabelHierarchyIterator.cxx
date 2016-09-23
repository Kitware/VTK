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
#include "vtkLabelHierarchyPrivate.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxSetObjectMacro(vtkLabelHierarchyIterator,Hierarchy,vtkLabelHierarchy);
vtkCxxSetObjectMacro(vtkLabelHierarchyIterator,TraversedBounds,vtkPolyData);

vtkLabelHierarchyIterator::vtkLabelHierarchyIterator()
{
  this->Hierarchy = 0;
  this->TraversedBounds = 0;
  this->BoundsFactor = 1.0;
  this->AllBounds = 0;
  this->AllBoundsRecorded = 0;
}

vtkLabelHierarchyIterator::~vtkLabelHierarchyIterator()
{
  if ( this->Hierarchy )
  {
    this->Hierarchy->Delete();
  }
  if ( this->TraversedBounds )
  {
    this->TraversedBounds->Delete();
  }
}

void vtkLabelHierarchyIterator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Hierarchy: " << this->Hierarchy << "\n";
  os << indent << "BoundsFactor: " << this->BoundsFactor << "\n";
  os << indent << "TraversedBounds: " << this->TraversedBounds << "\n";
  os << indent << "AllBounds: " << this->AllBounds << "\n";
  os << indent << "AllBoundsRecorded: " << this->AllBoundsRecorded << "\n";
}

void vtkLabelHierarchyIterator::GetPoint( double x[3] )
{
  this->GetHierarchy()->GetPoints()->GetPoint( this->GetLabelId(), x );
}

void vtkLabelHierarchyIterator::GetSize( double sz[2] )
{
  if ( ! this->GetHierarchy() )
  {
    sz[0] = sz[1] = 0.;
    return;
  }
  vtkDataArray* labelSizeArr = this->GetHierarchy()->GetSizes();
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
  if ( ! this->GetHierarchy() )
  {
    return -1;
  }
  vtkDataArray* labelTypeArr = this->GetHierarchy()->GetPointData()->GetArray( "Type" );
  if ( ! labelTypeArr )
  {
    return -1;
  }
  vtkIntArray* labelTypeIArr = vtkArrayDownCast<vtkIntArray>( labelTypeArr );
  if ( ! labelTypeIArr )
  {
    return -1;
  }
  if (labelTypeIArr->GetNumberOfTuples()==0)
  {
    return -1;
  }
  vtkIdType lid = this->GetLabelId();
  return labelTypeIArr->GetValue( lid );
}

vtkStdString vtkLabelHierarchyIterator::GetLabel()
{
  if ( ! this->GetHierarchy() )
  {
    return vtkStdString();
  }
  vtkAbstractArray* labelArr = this->GetHierarchy()->GetLabels();
  if (!labelArr)
  {
    return "";
  }
  return labelArr->GetVariantValue(this->GetLabelId()).ToString();
}

vtkUnicodeString vtkLabelHierarchyIterator::GetUnicodeLabel()
{
  if ( ! this->GetHierarchy() )
  {
    return vtkUnicodeString();
  }
  vtkAbstractArray* labelArr = this->GetHierarchy()->GetLabels();
  if ( ! labelArr )
  {
    return vtkUnicodeString();
  }
  return labelArr->GetVariantValue(this->GetLabelId()).ToUnicodeString();
}

double vtkLabelHierarchyIterator::GetOrientation()
{
  if ( ! this->GetHierarchy() )
  {
    return 0.0;
  }
  vtkDataArray* arr = this->GetHierarchy()->GetOrientations();
  if ( ! arr )
  {
    return 0.0;
  }
  return arr->GetTuple1(this->GetLabelId());
}

void vtkLabelHierarchyIterator::GetBoundedSize( double sz[2] )
{
  if ( ! this->GetHierarchy() )
  {
    sz[0] = sz[1] = 0.;
    return;
  }
  vtkDataArray* boundedSizeArr = this->GetHierarchy()->GetBoundedSizes();
  if ( ! boundedSizeArr )
  {
    sz[0] = sz[1] = 0.;
    return;
  }
  vtkIdType lid = this->GetLabelId();
  double* ls = boundedSizeArr->GetTuple( lid );
  sz[0] = ls[0];
  sz[1] = ls[1];
}

void vtkLabelHierarchyIterator::BoxNode()
{
  if ( ! this->TraversedBounds || this->IsAtEnd() )
  {
    return;
  }

  if ( this->AllBounds )
  {
    if ( ! this->AllBoundsRecorded )
    {
      this->AllBoundsRecorded = 1;
      this->BoxAllNodes( this->TraversedBounds );
    }
    return;
  }

  double ctr[3];
  double sz;
  this->GetNodeGeometry( ctr, sz );
  double tf = this->BoundsFactor;

  if ( this->Hierarchy->GetImplementation()->Hierarchy3 )
  {
    this->BoxNodeInternal3( ctr, tf * sz );
  }
  else if ( this->Hierarchy->GetImplementation()->Hierarchy2 )
  {
    this->BoxNodeInternal2( ctr, tf * sz );
  }
}

void vtkLabelHierarchyIterator::BoxAllNodes( vtkPolyData* boxes )
{
  if ( ! boxes )
  {
    return;
  }

  vtkPolyData* tmp = this->TraversedBounds;
  this->TraversedBounds = boxes;
  double tf = this->BoundsFactor;

  if ( this->Hierarchy->GetImplementation()->Hierarchy3 )
  {
    vtkLabelHierarchy::Implementation::HierarchyIterator3 iter;
    for (
      iter = this->Hierarchy->GetImplementation()->Hierarchy3->begin( true );
      iter != this->Hierarchy->GetImplementation()->Hierarchy3->end( true );
      ++ iter )
    {
      this->BoxNodeInternal3( iter->value().GetCenter(), iter->value().GetSize() / 2. * tf );
    }
  }
  else if ( this->Hierarchy->GetImplementation()->Hierarchy2 )
  {
    vtkLabelHierarchy::Implementation::HierarchyIterator2 iter;
    double ctr[3];
    double zvalfoo = this->Hierarchy->GetImplementation()->Z2;
    for (
      iter = this->Hierarchy->GetImplementation()->Hierarchy2->begin( true );
      iter != this->Hierarchy->GetImplementation()->Hierarchy2->end( true );
      ++ iter )
    {
      ctr[0] = iter->value().GetCenter()[0];
      ctr[1] = iter->value().GetCenter()[1];
      ctr[2] = zvalfoo;
      this->BoxNodeInternal2( ctr, iter->value().GetSize() / 2. * tf );
    }
  }

  this->TraversedBounds = tmp;
}

static const int vtkLabelHierarchyIteratorEdgeIds[12][2] =
{
  {  0,  1 },
  {  1,  2 },
  {  2,  3 },
  {  3,  0 },

  {  4,  5 },
  {  5,  6 },
  {  6,  7 },
  {  7,  4 },

  {  0,  4 },
  {  1,  5 },
  {  2,  6 },
  {  3,  7 },
};


void vtkLabelHierarchyIterator::BoxNodeInternal3( const double* ctr, double sz )
{
  vtkIdType conn[8];
  vtkPoints* pts = this->TraversedBounds->GetPoints();
  conn[0] = pts->InsertNextPoint( ctr[0] - sz, ctr[1] - sz, ctr[2] - sz );
  conn[1] = pts->InsertNextPoint( ctr[0] + sz, ctr[1] - sz, ctr[2] - sz );
  conn[2] = pts->InsertNextPoint( ctr[0] + sz, ctr[1] + sz, ctr[2] - sz );
  conn[3] = pts->InsertNextPoint( ctr[0] - sz, ctr[1] + sz, ctr[2] - sz );
  conn[4] = pts->InsertNextPoint( ctr[0] - sz, ctr[1] - sz, ctr[2] + sz );
  conn[5] = pts->InsertNextPoint( ctr[0] + sz, ctr[1] - sz, ctr[2] + sz );
  conn[6] = pts->InsertNextPoint( ctr[0] + sz, ctr[1] + sz, ctr[2] + sz );
  conn[7] = pts->InsertNextPoint( ctr[0] - sz, ctr[1] + sz, ctr[2] + sz );
  vtkIdType econn[2];
  for ( int i = 0; i < 12; ++ i )
  {
    econn[0] = conn[vtkLabelHierarchyIteratorEdgeIds[i][0]];
    econn[1] = conn[vtkLabelHierarchyIteratorEdgeIds[i][1]];
    this->TraversedBounds->InsertNextCell( VTK_LINE, 2, econn );
  }
}

void vtkLabelHierarchyIterator::BoxNodeInternal2( const double* ctr, double sz )
{
  vtkIdType conn[4];
  vtkPoints* pts = this->TraversedBounds->GetPoints();
  conn[0] = pts->InsertNextPoint( ctr[0] - sz, ctr[1] - sz, ctr[2] );
  conn[1] = pts->InsertNextPoint( ctr[0] + sz, ctr[1] - sz, ctr[2] );
  conn[2] = pts->InsertNextPoint( ctr[0] + sz, ctr[1] + sz, ctr[2] );
  conn[3] = pts->InsertNextPoint( ctr[0] - sz, ctr[1] + sz, ctr[2] );
  vtkIdType econn[2];
  for ( int i = 0; i < 4; ++ i )
  {
    econn[0] = conn[vtkLabelHierarchyIteratorEdgeIds[i][0]];
    econn[1] = conn[vtkLabelHierarchyIteratorEdgeIds[i][1]];
    this->TraversedBounds->InsertNextCell( VTK_LINE, 2, econn );
  }
}

