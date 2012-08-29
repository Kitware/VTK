/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkScalarBarRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkScalarBarActor.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

//=============================================================================
vtkStandardNewMacro(vtkScalarBarRepresentation);
//-----------------------------------------------------------------------------
vtkScalarBarRepresentation::vtkScalarBarRepresentation()
{
  this->PositionCoordinate->SetValue(0.82, 0.1);
  this->Position2Coordinate->SetValue(0.17, 0.8);

  this->ScalarBarActor = NULL;
  vtkScalarBarActor *actor = vtkScalarBarActor::New();
  this->SetScalarBarActor(actor);
  actor->Delete();

  this->ShowBorder = vtkBorderRepresentation::BORDER_ACTIVE;
  this->BWActor->VisibilityOff();
}

//-----------------------------------------------------------------------------
vtkScalarBarRepresentation::~vtkScalarBarRepresentation()
{
  this->SetScalarBarActor(NULL);
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::SetScalarBarActor(vtkScalarBarActor* actor)
{
  if (this->ScalarBarActor != actor)
    {
    vtkSmartPointer<vtkScalarBarActor> oldActor = this->ScalarBarActor;
    vtkSetObjectBodyMacro(ScalarBarActor, vtkScalarBarActor, actor);
    if (actor && oldActor)
      {
      actor->SetOrientation(oldActor->GetOrientation());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ScalarBarActor: " << this->ScalarBarActor << endl;
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::SetOrientation(int orientation)
{
  if (this->ScalarBarActor)
    {
    this->ScalarBarActor->SetOrientation(orientation);
    }
}

//-----------------------------------------------------------------------------
int vtkScalarBarRepresentation::GetOrientation()
{
  if (this->ScalarBarActor)
    {
    return this->ScalarBarActor->GetOrientation();
    }
  vtkErrorMacro("No scalar bar");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::BuildRepresentation()
{
  if (this->ScalarBarActor)
    {
    this->ScalarBarActor->SetPosition(this->GetPosition());
    this->ScalarBarActor->SetPosition2(this->GetPosition2());
    }

  this->Superclass::BuildRepresentation();
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::WidgetInteraction(double eventPos[2])
{
  // Let superclass move things around.
  this->Superclass::WidgetInteraction(eventPos);

  // Check to see if we need to change the orientation.
  double *fpos1 = this->PositionCoordinate->GetValue();
  double *fpos2 = this->Position2Coordinate->GetValue();
  double par1[2];
  double par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];
  double center[2];
  center[0] = fpos1[0] + 0.5*fpos2[0];
  center[1] = fpos1[1] + 0.5*fpos2[1];
  bool orientationSwapped = false;
  if (fabs(center[0] - 0.5) > 0.2+fabs(center[1] - 0.5))
    {
    // Close enough to left/right to be swapped to vertical
    if (this->ScalarBarActor->GetOrientation() == VTK_ORIENT_HORIZONTAL)
      {
      this->ScalarBarActor->SetOrientation(VTK_ORIENT_VERTICAL);
      orientationSwapped = true;
      }
    }
  else if (fabs(center[1] - 0.5) > 0.2+fabs(center[0] - 0.5))
    {
    // Close enough to left/right to be swapped to horizontal
    if (this->ScalarBarActor->GetOrientation() == VTK_ORIENT_VERTICAL)
      {
      this->ScalarBarActor->SetOrientation(VTK_ORIENT_HORIZONTAL);
      orientationSwapped = true;
      }
    }

  if (orientationSwapped)
    {
    // Change the corners to effectively rotate 90 degrees.
    par2[0] = center[0] + center[1] - par1[1];
    par2[1] = center[1] + center[0] - par1[0];
    par1[0] = 2*center[0] - par2[0];
    par1[1] = 2*center[1] - par2[1];

    this->PositionCoordinate->SetValue(par1[0],par1[1]);
    this->Position2Coordinate->SetValue(par2[0] - par1[0], par2[1] - par1[1]);

    this->Modified();
    this->BuildRepresentation();
  }
}

//-----------------------------------------------------------------------------
int vtkScalarBarRepresentation::GetVisibility()
{
  return this->ScalarBarActor->GetVisibility();
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::SetVisibility(int vis)
{
  this->ScalarBarActor->SetVisibility(vis);
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::GetActors2D(vtkPropCollection *collection)
{
  if (this->ScalarBarActor)
    {
    collection->AddItem(this->ScalarBarActor);
    }
  this->Superclass::GetActors2D(collection);
}

//-----------------------------------------------------------------------------
void vtkScalarBarRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  if (this->ScalarBarActor)
    {
    this->ScalarBarActor->ReleaseGraphicsResources(w);
    }
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkScalarBarRepresentation::RenderOverlay(vtkViewport *w)
{
  int count = this->Superclass::RenderOverlay(w);
  if (this->ScalarBarActor)
    {
    count += this->ScalarBarActor->RenderOverlay(w);
    }
  return count;
}

//-------------------------------------------------------------------------
int vtkScalarBarRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  if (this->ScalarBarActor)
    {
    count += this->ScalarBarActor->RenderOpaqueGeometry(w);
    }
  return count;
}

//-------------------------------------------------------------------------
int vtkScalarBarRepresentation::RenderTranslucentPolygonalGeometry(
                                                                 vtkViewport *w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  if (this->ScalarBarActor)
    {
    count += this->ScalarBarActor->RenderTranslucentPolygonalGeometry(w);
    }
  return count;
}

//-------------------------------------------------------------------------
int vtkScalarBarRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->ScalarBarActor)
    {
    result |= this->ScalarBarActor->HasTranslucentPolygonalGeometry();
    }
  return result;
}

