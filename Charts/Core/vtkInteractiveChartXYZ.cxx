/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractiveChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractiveChartXYZ.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkChartXYZPrivate.h"
#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkPen.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkIdTypeArray.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <cassert>

using std::vector;

vtkStandardNewMacro(vtkInteractiveChartXYZ)

void vtkInteractiveChartXYZ::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

void vtkInteractiveChartXYZ::Update()
{
  if (this->Link)
    {
    // Copy the row numbers so that we can do the highlight...
    if (!d->points.empty())
      {
      vtkSelection *selection =
          vtkSelection::SafeDownCast(this->Link->GetOutputDataObject(2));
      if (selection->GetNumberOfNodes())
        {
        vtkSelectionNode *node = selection->GetNode(0);
        vtkIdTypeArray *idArray =
            vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
        if (d->selectedPointsBuidTime > idArray->GetMTime() ||
            this->GetMTime() > d->selectedPointsBuidTime)
          {
          d->selectedPoints.resize(idArray->GetNumberOfTuples());
          for (vtkIdType i = 0; i < idArray->GetNumberOfTuples(); ++i)
            {
            d->selectedPoints[i] = d->points[idArray->GetValue(i)];
            }
          d->selectedPointsBuidTime.Modified();
          }
        }
      }
    }
}

bool vtkInteractiveChartXYZ::Paint(vtkContext2D *painter)
{
  if (!this->Visible || d->points.size() == 0)
    return false;

  // Get the 3D context.
  vtkContext3D *context = painter->GetContext3D();

  if (!context)
    return false;

  this->Update();

  // Calculate the transforms required for the current rotation.
  d->CalculateTransforms();

  context->PushMatrix();
  context->AppendTransform(d->Rotation.GetPointer());

  // First lets draw the points in 3d.
  context->ApplyPen(this->Pen.GetPointer());
  context->DrawPoints(d->points[0].GetData(), d->points.size());

  // Now to render the selected points.
  if (!d->selectedPoints.empty())
    {
    context->ApplyPen(this->SelectedPen.GetPointer());
    context->DrawPoints(d->selectedPoints[0].GetData(), d->selectedPoints.size());
    }

  context->PopMatrix();

  // Now to draw the axes - pretty basic for now but could be extended.
  context->PushMatrix();
  context->AppendTransform(d->Box.GetPointer());
  context->ApplyPen(this->AxisPen.GetPointer());

  vtkVector3f box[4];
  box[0] = vtkVector3f(0, 0, 0);
  box[1] = vtkVector3f(0, 1, 0);
  box[2] = vtkVector3f(1, 1, 0);
  box[3] = vtkVector3f(1, 0, 0);
  context->DrawLine(box[0], box[1]);
  context->DrawLine(box[1], box[2]);
  context->DrawLine(box[2], box[3]);
  context->DrawLine(box[3], box[0]);
  for (int i = 0; i < 4; ++i)
    {
    box[i].SetZ(1);
    }
  context->DrawLine(box[0], box[1]);
  context->DrawLine(box[1], box[2]);
  context->DrawLine(box[2], box[3]);
  context->DrawLine(box[3], box[0]);
  context->DrawLine(vtkVector3f(0, 0, 0), vtkVector3f(0, 0, 1));
  context->DrawLine(vtkVector3f(1, 0, 0), vtkVector3f(1, 0, 1));
  context->DrawLine(vtkVector3f(0, 1, 0), vtkVector3f(0, 1, 1));
  context->DrawLine(vtkVector3f(1, 1, 0), vtkVector3f(1, 1, 1));
  context->PopMatrix();

  return true;
}

void vtkInteractiveChartXYZ::SetInput(vtkTable *input, const vtkStdString &xName,
                           const vtkStdString &yName, const vtkStdString &zName,
                           const vtkStdString &colorName)
{
  this->Superclass::SetInput(input, xName, yName, zName);
}

vtkInteractiveChartXYZ::vtkInteractiveChartXYZ()
{
  this->Interactive = true;
}

vtkInteractiveChartXYZ::~vtkInteractiveChartXYZ()
{
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Determine anchor to zoom in on
    vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    vtkVector2d pos(0.0, 0.0);
    vtkTransform *transform = this->d->Transform.GetPointer();
    //transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    //this->ZoomAnchor = vtkVector2f(pos.Cast<float>().GetData());
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    vtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());

    double dx = screenPos[0] - lastScreenPos[0];
    double dy = screenPos[1] - lastScreenPos[1];
    double dist = sqrt(dx*dx + dy*dy);
  
    double delta_elevation = -20.0 / this->Scene->GetSceneHeight();
    double delta_azimuth = -20.0 / this->Scene->GetSceneWidth();

    double rxf = dx * delta_azimuth * 10.0;
    double ryf = dy * delta_elevation * 10.0;
    
    double w = dist * (delta_azimuth + delta_elevation) / 2 * 10.0;



    this->d->Rotate->RotateY(-rxf);
    this->d->Rotate->RotateX(ryf);

/*
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Azimuth(rxf);
  camera->Elevation(ryf);
  camera->OrthogonalizeViewUp();

    // Go from screen to scene coordinates to work out the delta
    vtkTransform *transform = this->d->Transform.GetPointer();
    //transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    //transform->InverseTransformPoints(lastScreenPos.GetData(), last.GetData(), 1);
    vtkVector2f delta((last - pos).Cast<float>().GetData());
    this->Translate(-delta[0], -delta[1]);
  */

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
    }
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
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
    /*
    this->Translate(this->ZoomAnchor[0], this->ZoomAnchor[1]);
    this->Scale(scaling, scaling);
    this->Translate(-this->ZoomAnchor[0], -this->ZoomAnchor[1]);
    */

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(vtkCommand::InteractionEvent);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkInteractiveChartXYZ::MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta)
{
  // Determine current position to zoom in on
  /*
  vtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
  vtkVector2d pos(0.0, 0.0);
  vtkTransform2D *transform = this->GetTransform();
  transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
  vtkVector2f zoomAnchor = vtkVector2f(pos.Cast<float>().GetData());
  */

  // Ten "wheels" to double/halve zoom level
  float scaling = pow(2.0f, delta/10.0f);

  // Zoom in on current position
  /*
  this->Translate(zoomAnchor[0], zoomAnchor[1]);
  this->Scale(scaling, scaling);
  this->Translate(-zoomAnchor[0], -zoomAnchor[1]);
  */

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  this->InvokeEvent(vtkCommand::InteractionEvent);
  return true;
}
