/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextTransform.h"

#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScenePrivate.h"
#include "vtkObjectFactory.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkContextTransform);

//-----------------------------------------------------------------------------
vtkContextTransform::vtkContextTransform() : ZoomAnchor(0.0f, 0.0f)
{
  this->Transform = vtkSmartPointer<vtkTransform2D>::New();
  this->PanMouseButton = vtkContextMouseEvent::LEFT_BUTTON;
  this->PanModifier = vtkContextMouseEvent::NO_MODIFIER;
  this->ZoomMouseButton = vtkContextMouseEvent::RIGHT_BUTTON;
  this->ZoomModifier = vtkContextMouseEvent::NO_MODIFIER;
  this->SecondaryPanMouseButton = vtkContextMouseEvent::NO_BUTTON;
  this->SecondaryPanModifier = vtkContextMouseEvent::NO_MODIFIER;
  this->SecondaryZoomMouseButton = vtkContextMouseEvent::LEFT_BUTTON;
  this->SecondaryZoomModifier = vtkContextMouseEvent::SHIFT_MODIFIER;

  this->ZoomOnMouseWheel = true;
  this->PanYOnMouseWheel = false;

  this->Interactive = false;
}

//-----------------------------------------------------------------------------
vtkContextTransform::~vtkContextTransform()
{
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::Paint(vtkContext2D *painter)
{
  painter->PushMatrix();
  painter->AppendTransform(this->Transform);
  bool result = this->PaintChildren(painter);
  painter->PopMatrix();
  return result;
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Update()
{
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Translate(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Translate(d);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Scale(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Scale(d);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Rotate(float angle)
{
  this->Transform->Rotate(double(angle));
}

//-----------------------------------------------------------------------------
vtkTransform2D* vtkContextTransform::GetTransform()
{
  return this->Transform;
}

//-----------------------------------------------------------------------------
vtkVector2f vtkContextTransform::MapToParent(const vtkVector2f& point)
{
  vtkVector2f p;
  this->Transform->TransformPoints(point.GetData(), p.GetData(), 1);
  return p;
}

//-----------------------------------------------------------------------------
vtkVector2f vtkContextTransform::MapFromParent(const vtkVector2f& point)
{
  vtkVector2f p;
  this->Transform->InverseTransformPoints(point.GetData(), p.GetData(), 1);
  return p;
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (!this->Interactive)
    {
    return vtkAbstractContextItem::MouseButtonPressEvent(mouse);
    }
  if ((this->ZoomMouseButton != vtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->ZoomMouseButton &&
        mouse.GetModifiers() == this->ZoomModifier) ||
      (this->SecondaryZoomMouseButton != vtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->SecondaryZoomMouseButton &&
        mouse.GetModifiers() == this->SecondaryZoomModifier) )
    {
    // Determine anchor to zoom in on
    vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    vtkVector2d pos(0.0, 0.0);
    vtkTransform2D *transform = this->GetTransform();
    transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    this->ZoomAnchor = vtkVector2f(pos.Cast<float>().GetData());
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (!this->Interactive)
    {
    return vtkAbstractContextItem::MouseButtonPressEvent(mouse);
    }
  if ((this->PanMouseButton != vtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->PanMouseButton &&
        mouse.GetModifiers() == this->PanModifier) ||
      (this->SecondaryPanMouseButton != vtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->SecondaryPanMouseButton &&
        mouse.GetModifiers() == this->SecondaryPanModifier) )
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());
    vtkVector2d pos(0.0, 0.0);
    vtkVector2d last(0.0, 0.0);

    // Go from screen to scene coordinates to work out the delta
    vtkTransform2D *transform = this->GetTransform();
    transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    transform->InverseTransformPoints(lastScreenPos.GetData(), last.GetData(), 1);
    vtkVector2f delta((last - pos).Cast<float>().GetData());
    this->Translate(-delta[0], -delta[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
    }
  if ((this->ZoomMouseButton != vtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->ZoomMouseButton &&
        mouse.GetModifiers() == this->ZoomModifier) ||
      (this->SecondaryZoomMouseButton != vtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->SecondaryZoomMouseButton &&
        mouse.GetModifiers() == this->SecondaryZoomModifier) )
    {
    // Figure out how much the mouse has moved and scale accordingly
    float delta = 0.0f;
    if (this->Scene->GetSceneHeight() > 0)
      {
      delta = static_cast<float>(mouse.GetLastScreenPos()[1] - mouse.GetScreenPos()[1])/this->Scene->GetSceneHeight();
      }

    // Dragging full screen height zooms 4x.
    float scaling = pow(4.0f, delta);

    // Zoom in on anchor position
    this->Translate(this->ZoomAnchor[0], this->ZoomAnchor[1]);
    this->Scale(scaling, scaling);
    this->Translate(-this->ZoomAnchor[0], -this->ZoomAnchor[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta)
{
  if (!this->Interactive)
    {
    return vtkAbstractContextItem::MouseButtonPressEvent(mouse);
    }
  if (this->ZoomOnMouseWheel)
    {
    // Determine current position to zoom in on
    vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    vtkVector2d pos(0.0, 0.0);
    vtkTransform2D *transform = this->GetTransform();
    transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    vtkVector2f zoomAnchor = vtkVector2f(pos.Cast<float>().GetData());

    // Ten "wheels" to double/halve zoom level
    float scaling = pow(2.0f, delta/10.0f);

    // Zoom in on current position
    this->Translate(zoomAnchor[0], zoomAnchor[1]);
    this->Scale(scaling, scaling);
    this->Translate(-zoomAnchor[0], -zoomAnchor[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
    }
  if (this->PanYOnMouseWheel)
    {
    // Ten "wheels" to scroll a screen
    this->Translate(0.0f, delta/10.0f*this->Scene->GetSceneHeight());

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkContextTransform::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
