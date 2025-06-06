// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkResliceCursorActor.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkViewport.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkResliceCursorActor);

//------------------------------------------------------------------------------
vtkResliceCursorActor::vtkResliceCursorActor()
{
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

  this->CenterlineProperty[0]->SetColor(1, 0, 0);
  this->CenterlineProperty[1]->SetColor(0, 1, 0);
  this->CenterlineProperty[2]->SetColor(0, 0, 1);
  this->ThickSlabProperty[0]->SetColor(1, 0.6, 0.6);
  this->ThickSlabProperty[1]->SetColor(0.6, 1, 0.6);
  this->ThickSlabProperty[2]->SetColor(0.6, 0.6, 1);

  this->CenterlineProperty[0]->SetEdgeColor(1, 0, 0);
  this->CenterlineProperty[1]->SetEdgeColor(0, 1, 0);
  this->CenterlineProperty[2]->SetEdgeColor(0, 0, 1);
  this->ThickSlabProperty[0]->SetEdgeColor(1, 0.6, 0.6);
  this->ThickSlabProperty[1]->SetEdgeColor(0.6, 1, 0.6);
  this->ThickSlabProperty[2]->SetEdgeColor(0.6, 0.6, 1);

  this->CenterlineProperty[0]->SetEdgeVisibility(1);
  this->CenterlineProperty[1]->SetEdgeVisibility(1);
  this->CenterlineProperty[2]->SetEdgeVisibility(1);
  this->ThickSlabProperty[0]->SetEdgeVisibility(1);
  this->ThickSlabProperty[1]->SetEdgeVisibility(1);
  this->ThickSlabProperty[2]->SetEdgeVisibility(1);
}

//------------------------------------------------------------------------------
vtkResliceCursorActor::~vtkResliceCursorActor()
{
  for (int i = 0; i < 3; i++)
  {
    this->CursorCenterlineMapper[i]->Delete();
    this->CursorThickSlabMapper[i]->Delete();
    this->SetCenterlineActor(i, nullptr);
    this->SetThickSlabActor(i, nullptr);
  }
}

//------------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int vtkResliceCursorActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int result = 0;

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

//------------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
vtkTypeBool vtkResliceCursorActor::HasTranslucentPolygonalGeometry()
{
  return false;
}

//------------------------------------------------------------------------------
void vtkResliceCursorActor::ReleaseGraphicsResources(vtkWindow* window)
{
  for (int i = 0; i < 3; i++)
  {
    this->CursorCenterlineActor[i]->ReleaseGraphicsResources(window);
    this->CursorThickSlabActor[i]->ReleaseGraphicsResources(window);
  }
}

//------------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* vtkResliceCursorActor::GetBounds()
{
  // we cannot initialize the Bounds the same way vtkBoundingBox does because
  // vtkProp3D::GetLength() does not check if the Bounds are initialized or
  // not and makes a call to sqrt(). This call to sqrt with invalid values
  // would raise a floating-point overflow exception (notably on BCC).
  // As vtkMath::UninitializeBounds initialized finite invalid bounds, it
  // passes silently and GetLength() returns 0.
  vtkMath::UninitializeBounds(this->Bounds);

  this->UpdateViewProps();

  vtkBoundingBox* bb = new vtkBoundingBox();

  double bounds[6];
  for (int i = 0; i < 3; i++)
  {
    if (this->CursorCenterlineActor[i]->GetVisibility() &&
      this->CursorCenterlineActor[i]->GetUseBounds())
    {
      this->CursorCenterlineActor[i]->GetBounds(bounds);
      bb->AddBounds(bounds);
    }
    if (this->CursorThickSlabActor[i]->GetVisibility() &&
      this->CursorThickSlabActor[i]->GetUseBounds())
    {
      this->CursorThickSlabActor[i]->GetBounds(bounds);
      bb->AddBounds(bounds);
    }
  }

  bb->GetBounds(this->Bounds);

  delete bb;
  return this->Bounds;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkResliceCursorActor::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->CursorAlgorithm)
  {
    vtkMTimeType time;
    time = this->CursorAlgorithm->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}

//------------------------------------------------------------------------------
vtkProperty* vtkResliceCursorActor::GetCenterlineProperty(int i)
{
  return this->CenterlineProperty[i];
}

//------------------------------------------------------------------------------
vtkProperty* vtkResliceCursorActor::GetThickSlabProperty(int i)
{
  return this->ThickSlabProperty[i];
}

//------------------------------------------------------------------------------
void vtkResliceCursorActor::UpdateHoleSize(vtkViewport* v)
{
  vtkResliceCursor* r = this->CursorAlgorithm->GetResliceCursor();

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
    v->GetWorldPoint(wCenterHoleWidthAway);

    const double holeWidth =
      2.0 * sqrt(vtkMath::Distance2BetweenPoints(wCenter, wCenterHoleWidthAway));

    if (fabs(r->GetHoleWidth() - holeWidth) > 1e-5)
    {
      r->SetHoleWidth(holeWidth);
    }

    // MTime checks ensure that this will update only if the hole width
    // has actually changed.
    this->CursorAlgorithm->Update();
  }
}

//------------------------------------------------------------------------------
void vtkResliceCursorActor::UpdateViewProps(vtkViewport* v)
{
  if (this->CursorAlgorithm->GetResliceCursor() == nullptr)
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

  this->CursorCenterlineMapper[axis1]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(0));
  this->CursorCenterlineMapper[axis2]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(1));

  const bool thickMode = this->CursorAlgorithm->GetResliceCursor()->GetThickMode() != 0;

  if (thickMode)
  {
    this->CursorThickSlabMapper[axis1]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(2));
    this->CursorThickSlabMapper[axis2]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(3));

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

//------------------------------------------------------------------------------
void vtkResliceCursorActor::SetUserMatrix(vtkMatrix4x4* m)
{
  this->CursorThickSlabActor[0]->SetUserMatrix(m);
  this->CursorThickSlabActor[1]->SetUserMatrix(m);
  this->CursorThickSlabActor[2]->SetUserMatrix(m);
  this->CursorCenterlineActor[0]->SetUserMatrix(m);
  this->CursorCenterlineActor[1]->SetUserMatrix(m);
  this->CursorCenterlineActor[2]->SetUserMatrix(m);

  this->Superclass::SetUserMatrix(m);
}

//------------------------------------------------------------------------------
vtkActor* vtkResliceCursorActor::GetCenterlineActor(int axis)
{
  return this->CursorCenterlineActor[axis];
}

//------------------------------------------------------------------------------
void vtkResliceCursorActor::SetCenterlineActor(int axis, vtkActor* actor)
{
  if (this->CursorCenterlineActor[axis] != actor)
  {
    if (this->CursorCenterlineActor[axis] != nullptr)
    {
      this->CursorCenterlineActor[axis]->UnRegister(nullptr);
    }
    this->CursorCenterlineActor[axis] = actor;
    vtkProperty* property = nullptr;
    if (actor != nullptr)
    {
      actor->Register(nullptr);
      property = actor->GetProperty();
    }

    if (this->CenterlineProperty[axis] != property)
    {
      if (this->CenterlineProperty[axis] != nullptr)
      {
        this->CenterlineProperty[axis]->UnRegister(nullptr);
      }
      this->CenterlineProperty[axis] = property;
      if (property != nullptr)
      {
        property->Register(nullptr);
      }
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkActor* vtkResliceCursorActor::GetThickSlabActor(int axis)
{
  return this->CursorThickSlabActor[axis];
}

//------------------------------------------------------------------------------
void vtkResliceCursorActor::SetThickSlabActor(int axis, vtkActor* actor)
{
  if (this->CursorThickSlabActor[axis] != actor)
  {
    if (this->CursorThickSlabActor[axis] != nullptr)
    {
      this->CursorThickSlabActor[axis]->UnRegister(nullptr);
    }
    this->CursorThickSlabActor[axis] = actor;
    vtkProperty* property = nullptr;
    if (actor != nullptr)
    {
      actor->Register(nullptr);
      property = actor->GetProperty();
    }

    if (this->ThickSlabProperty[axis] != property)
    {
      if (this->ThickSlabProperty[axis] != nullptr)
      {
        this->ThickSlabProperty[axis]->UnRegister(nullptr);
      }
      this->ThickSlabProperty[axis] = property;
      if (property != nullptr)
      {
        property->Register(nullptr);
      }
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Prints an object if it exists.
#define vtkPrintMemberObjectMacro(obj, os, indent)                                                 \
  do                                                                                               \
  {                                                                                                \
    os << (indent) << #obj << ": ";                                                                \
    if (this->obj)                                                                                 \
    {                                                                                              \
      os << this->obj << "\n";                                                                     \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      os << "(null)\n";                                                                            \
    }                                                                                              \
  } while (false)

//------------------------------------------------------------------------------
void vtkResliceCursorActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkPrintMemberObjectMacro(CursorCenterlineActor[0], os, indent);
  vtkPrintMemberObjectMacro(CursorCenterlineActor[1], os, indent);
  vtkPrintMemberObjectMacro(CursorCenterlineActor[2], os, indent);
  vtkPrintMemberObjectMacro(CursorAlgorithm, os, indent);

  // this->CursorCenterlineMapper[3];
  // this->CursorCenterlineActor[3];
  // this->CursorThickSlabMapper[3];
  // this->CursorThickSlabActor[3];
  // this->CenterlineProperty[3];
  // this->ThickSlabProperty[3];
  // this->CursorAlgorithm;
}
VTK_ABI_NAMESPACE_END
