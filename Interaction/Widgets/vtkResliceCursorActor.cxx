/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkResliceCursorActor.h"

#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkPlane.h"
#include "vtkViewport.h"
#include "vtkResliceCursor.h"
#include "vtkProperty.h"
#include "vtkObjectFactory.h"
#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkResliceCursorActor);


// ----------------------------------------------------------------------------
vtkResliceCursorActor::vtkResliceCursorActor()
{
  this->CursorAlgorithm = vtkResliceCursorPolyDataAlgorithm::New();

  for (int i = 0; i < 3; i++)
    {
    this->CursorCenterlineMapper[i] = vtkPolyDataMapper::New();
    this->CursorCenterlineActor[i] = vtkActor::New();
    this->CursorThickSlabMapper[i] = vtkPolyDataMapper::New();
    this->CursorThickSlabActor[i] = vtkActor::New();
    this->CursorCenterlineMapper[i]->ScalarVisibilityOff();
    this->CursorThickSlabMapper[i]->ScalarVisibilityOff();

    this->CursorCenterlineActor[i]->SetMapper(this->CursorCenterlineMapper[i]);
    this->CursorThickSlabActor[i]->SetMapper(this->CursorThickSlabMapper[i]);
    this->CenterlineProperty[i] = vtkProperty::New();
    this->ThickSlabProperty[i] = vtkProperty::New();

    this->CursorCenterlineActor[i]->SetProperty(this->CenterlineProperty[i]);
    this->CursorThickSlabActor[i]->SetProperty(this->ThickSlabProperty[i]);
    }


  this->CenterlineProperty[0]->SetColor(1,0,0);
  this->CenterlineProperty[1]->SetColor(0,1,0);
  this->CenterlineProperty[2]->SetColor(0,0,1);
  this->ThickSlabProperty[0]->SetColor(1,0.6,0.6);
  this->ThickSlabProperty[1]->SetColor(0.6,1,0.6);
  this->ThickSlabProperty[2]->SetColor(0.6,0.6,1);

  this->CenterlineProperty[0]->SetEdgeColor(1,0,0);
  this->CenterlineProperty[1]->SetEdgeColor(0,1,0);
  this->CenterlineProperty[2]->SetEdgeColor(0,0,1);
  this->ThickSlabProperty[0]->SetEdgeColor(1,0.6,0.6);
  this->ThickSlabProperty[1]->SetEdgeColor(0.6,1,0.6);
  this->ThickSlabProperty[2]->SetEdgeColor(0.6,0.6,1);

  this->CenterlineProperty[0]->SetEdgeVisibility(1);
  this->CenterlineProperty[1]->SetEdgeVisibility(1);
  this->CenterlineProperty[2]->SetEdgeVisibility(1);
  this->ThickSlabProperty[0]->SetEdgeVisibility(1);
  this->ThickSlabProperty[1]->SetEdgeVisibility(1);
  this->ThickSlabProperty[2]->SetEdgeVisibility(1);
}

// ----------------------------------------------------------------------------
vtkResliceCursorActor::~vtkResliceCursorActor()
{
  for (int i = 0; i < 3; i++)
    {
    this->CursorCenterlineMapper[i]->Delete();
    this->CursorCenterlineActor[i]->Delete();
    this->CursorThickSlabMapper[i]->Delete();
    this->CursorThickSlabActor[i]->Delete();
    this->CenterlineProperty[i]->Delete();
    this->ThickSlabProperty[i]->Delete();
    }
  this->CursorAlgorithm->Delete();
}

// ----------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int vtkResliceCursorActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int result=0;

  if (this->CursorAlgorithm->GetResliceCursor())
    {
    this->UpdateViewProps(viewport);


    for (int i = 0; i < 3; i++)
      {
      if (this->CursorCenterlineActor[i]->GetVisibility())
        {
        result += this->CursorCenterlineActor[i]->RenderOpaqueGeometry(viewport);
        }
      if (this->CursorThickSlabActor[i]->GetVisibility())
        {
        result += this->CursorThickSlabActor[i]->RenderOpaqueGeometry(viewport);
        }
      }
    }

  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
int vtkResliceCursorActor::HasTranslucentPolygonalGeometry()
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkResliceCursorActor::ReleaseGraphicsResources(vtkWindow *window)
{
  for (int i = 0; i < 3; i++)
    {
    this->CursorCenterlineActor[i]->ReleaseGraphicsResources(window);
    this->CursorThickSlabActor[i]->ReleaseGraphicsResources(window);
    }
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkResliceCursorActor::GetBounds()
{
  // we cannot initialize the Bounds the same way vtkBoundingBox does because
  // vtkProp3D::GetLength() does not check if the Bounds are initialized or
  // not and makes a call to sqrt(). This call to sqrt with invalid values
  // would raise a floating-point overflow exception (notably on BCC).
  // As vtkMath::UninitializeBounds initialized finite unvalid bounds, it
  // passes silently and GetLength() returns 0.
  vtkMath::UninitializeBounds(this->Bounds);

  this->UpdateViewProps();

  vtkBoundingBox *bb = new vtkBoundingBox();

  double bounds[6];
  for (int i = 0; i < 3; i++)
    {
    if (this->CursorCenterlineActor[i]->GetVisibility()
        && this->CursorCenterlineActor[i]->GetUseBounds())
      {
      this->CursorCenterlineActor[i]->GetBounds(bounds);
      bb->AddBounds(bounds);
      }
    if (this->CursorThickSlabActor[i]->GetVisibility()
        && this->CursorThickSlabActor[i]->GetUseBounds())
      {
      this->CursorThickSlabActor[i]->GetBounds(bounds);
      bb->AddBounds(bounds);
      }
    }

  bb->GetBounds(this->Bounds);

  delete bb;
  return this->Bounds;
}

//-------------------------------------------------------------------------
unsigned long int vtkResliceCursorActor::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  if (this->CursorAlgorithm)
    {
    unsigned long time;
    time = this->CursorAlgorithm->GetMTime();
    if (time > mTime)
      {
      mTime = time;
      }
    }

  return mTime;
}

// ----------------------------------------------------------------------------
vtkProperty *vtkResliceCursorActor::GetCenterlineProperty( int i )
{
  return this->CenterlineProperty[i];
}

// ----------------------------------------------------------------------------
vtkProperty *vtkResliceCursorActor::GetThickSlabProperty( int i )
{
  return this->ThickSlabProperty[i];
}

// ----------------------------------------------------------------------------
void vtkResliceCursorActor::UpdateHoleSize( vtkViewport * v )
{
  vtkResliceCursor * r = this->CursorAlgorithm->GetResliceCursor();

  if (r->GetHoleWidthInPixels() && r->GetHole() && v)
    {

    // Get the reslice center in display coords.

    double dCenter[4], wCenter[4], wCenterHoleWidthAway[4];
    r->GetCenter(wCenter);
    wCenter[3] = 1.0;
    v->SetWorldPoint(wCenter);
    v->WorldToDisplay();
    v->GetDisplayPoint(dCenter);

    // Get the world position of a point "holeWidth pixels" away from the
    // reslice center

    dCenter[0] += (r->GetHoleWidthInPixels() / 2.0);
    v->SetDisplayPoint(dCenter);
    v->DisplayToWorld();
    v->GetWorldPoint( wCenterHoleWidthAway );

    const double holeWidth = 2.0 *
      sqrt( vtkMath::Distance2BetweenPoints( wCenter, wCenterHoleWidthAway ));
    r->SetHoleWidth(holeWidth);

    // MTime checks ensure that this will update only if the hole width
    // has actually changed.
    this->CursorAlgorithm->Update();
    }
}

// ----------------------------------------------------------------------------
void vtkResliceCursorActor::UpdateViewProps(vtkViewport *v)
{
  if (this->CursorAlgorithm->GetResliceCursor() == 0)
    {
    vtkDebugMacro(<< "no cursor to represent.");
    return;
    }

  this->CursorAlgorithm->Update();

  // Update the cursor to reflect a constant hole size in pixels, if necessary

  this->UpdateHoleSize(v);

  // Rebuild cursor to have the right hole with if necessary

  const int axisNormal = this->CursorAlgorithm->GetReslicePlaneNormal();
  const int axis1 = this->CursorAlgorithm->GetPlaneAxis1();
  const int axis2 = this->CursorAlgorithm->GetPlaneAxis2();

  this->CursorCenterlineMapper[axis1]->SetInputConnection(
        this->CursorAlgorithm->GetOutputPort(0));
  this->CursorCenterlineMapper[axis2]->SetInputConnection(
        this->CursorAlgorithm->GetOutputPort(1));

  const bool thickMode =
    this->CursorAlgorithm->GetResliceCursor()->GetThickMode() ? true : false;

  if (thickMode)
    {
    this->CursorThickSlabMapper[axis1]->SetInputConnection(
        this->CursorAlgorithm->GetOutputPort(2));
    this->CursorThickSlabMapper[axis2]->SetInputConnection(
        this->CursorAlgorithm->GetOutputPort(3));

    this->CursorThickSlabActor[axis1]->SetVisibility(1);
    this->CursorThickSlabActor[axis2]->SetVisibility(1);
    }

  this->CursorThickSlabActor[axis1]->SetVisibility(thickMode);
  this->CursorThickSlabActor[axis2]->SetVisibility(thickMode);
  this->CursorThickSlabActor[axisNormal]->SetVisibility(0);
  this->CursorCenterlineActor[axis1]->SetVisibility(1);
  this->CursorCenterlineActor[axis2]->SetVisibility(1);
  this->CursorCenterlineActor[axisNormal]->SetVisibility(0);

  this->CursorThickSlabActor[axis1]->GetProperty()->SetEdgeVisibility(thickMode);
  this->CursorThickSlabActor[axis2]->GetProperty()->SetEdgeVisibility(thickMode);
  this->CursorThickSlabActor[axisNormal]->GetProperty()->SetEdgeVisibility(0);
  this->CursorCenterlineActor[axis1]->GetProperty()->SetEdgeVisibility(1);
  this->CursorCenterlineActor[axis2]->GetProperty()->SetEdgeVisibility(1);
  this->CursorCenterlineActor[axisNormal]->GetProperty()->SetEdgeVisibility(0);
}

//----------------------------------------------------------------------
void vtkResliceCursorActor::SetUserMatrix(vtkMatrix4x4 *m)
{
  this->CursorThickSlabActor[0]->SetUserMatrix(m);
  this->CursorThickSlabActor[1]->SetUserMatrix(m);
  this->CursorThickSlabActor[2]->SetUserMatrix(m);
  this->CursorCenterlineActor[0]->SetUserMatrix(m);
  this->CursorCenterlineActor[1]->SetUserMatrix(m);
  this->CursorCenterlineActor[2]->SetUserMatrix(m);

  this->Superclass::SetUserMatrix(m);
}

//-------------------------------------------------------------------------
vtkActor * vtkResliceCursorActor::GetCenterlineActor( int axis )
{
  return this->CursorCenterlineActor[axis];
}

//----------------------------------------------------------------------
// Prints an object if it exists.
#define vtkPrintMemberObjectMacro( obj, os, indent ) \
  os << indent << #obj << ": "; \
  if (this->obj) \
    { \
    os << this->obj << "\n"; \
    } \
  else \
    { \
    os << "(null)\n"; \
    }

//-------------------------------------------------------------------------
void vtkResliceCursorActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  vtkPrintMemberObjectMacro( CursorCenterlineActor[0], os, indent );
  vtkPrintMemberObjectMacro( CursorCenterlineActor[1], os, indent );
  vtkPrintMemberObjectMacro( CursorCenterlineActor[2], os, indent );
  vtkPrintMemberObjectMacro( CursorAlgorithm, os, indent );

  // this->CursorCenterlineMapper[3];
  // this->CursorCenterlineActor[3];
  // this->CursorThickSlabMapper[3];
  // this->CursorThickSlabActor[3];
  // this->CenterlineProperty[3];
  // this->ThickSlabProperty[3];
  // this->CursorAlgorithm;
}
