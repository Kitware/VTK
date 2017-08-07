/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPanelRepresentation.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPanelRepresentation.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"
#include "vtkCamera.h"
#include "vtkPickingManager.h"
#include "vtkAssemblyPath.h"

#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkOpenVRPanelRepresentation);

//----------------------------------------------------------------------------
vtkOpenVRPanelRepresentation::vtkOpenVRPanelRepresentation()
{
  // The text
  this->Text = vtkStdString();

  this->TextActor = vtkTextActor3D::New();
  this->TextActor->GetTextProperty()->SetFontSize(17);
  this->TextActor->SetInput(this->Text.c_str());

  vtkTextProperty *prop = this->TextActor->GetTextProperty();
  this->TextActor->ForceOpaqueOn();

  prop->SetFontFamilyToTimes();
  prop->SetFrame(1);
  prop->SetFrameWidth(12);
  prop->SetFrameColor(0.0, 0.0, 0.0);
  prop->SetBackgroundOpacity(1.0);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(25);

  // Keep track of the hovered/picked prop
  this->HoveredProp = nullptr;

  this->VisibilityOff();
}

//----------------------------------------------------------------------------
vtkOpenVRPanelRepresentation::~vtkOpenVRPanelRepresentation()
{
  this->TextActor->Delete();
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::StartWidgetInteraction(double*)
{
  if (!this->HoveredProp || this->GetVisibility() )
  {
    return;
  }
  this->BuildRepresentation();
  this->VisibilityOn();
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::EndWidgetInteraction(double*)
{
  if (!this->GetVisibility())
  {
    return;
  }
  this->HoveredProp = nullptr;
  this->VisibilityOff();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->TextActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkOpenVRPanelRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  int count = 0;

  count += this->TextActor->RenderOpaqueGeometry(v);

  return count;
}

//-----------------------------------------------------------------------------
int vtkOpenVRPanelRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *v)
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  int count = 0;

  count += this->TextActor->RenderTranslucentPolygonalGeometry(v);

  return count;
}

//-----------------------------------------------------------------------------
int vtkOpenVRPanelRepresentation::HasTranslucentPolygonalGeometry()
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  int result = 0;

  result |= this->TextActor->HasTranslucentPolygonalGeometry();

  return result;
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime)
  {
    //Set Physical Scale
    if (this->Renderer && this->Renderer->GetRenderWindow() &&
      this->Renderer->GetRenderWindow()->GetInteractor())
    {
      vtkRenderWindowInteractor3D* iren =
        static_cast<vtkRenderWindowInteractor3D*>(
          this->Renderer->GetRenderWindow()->GetInteractor());
      if (iren)
      {
        this->PhysicalScale = iren->GetPhysicalScale();
      }
    }

    //Compute camera position and orientation
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    double* pos = cam->GetPosition();
    double* dop = cam->GetDirectionOfProjection();
    vtkMath::Normalize(dop);

    //Compute text size in world coordinates
    int bbox[4] = {0, 0, 0, 0};
    this->TextActor->GetBoundingBox(bbox);

    double offset = 0.05 * this->GetPhysicalScale();
    double PPI = 450;//Screen resolution in pixels per inch
    double FontSizeFactor = 1.0 / PPI;//Map font size to world coordinates

    double textSize[2] = {
      (bbox[1] - bbox[0]) * FontSizeFactor * this->GetPhysicalScale(),
      (bbox[3] - bbox[2]) * FontSizeFactor * this->GetPhysicalScale()};

    //Max frame distance to camera
    double frameDistance = 1.5 * this->GetPhysicalScale();
    //Min frame distance to camera
    double minFrameDistance = 0.3 *this->GetPhysicalScale();

    double frameSize[2] = {textSize[0] + offset, textSize[1] + offset};

    //Reduce frame distance if the prop is on the way
    if (this->HoveredProp)
    {
      double* bnds = this->HoveredProp->GetBounds();
      if (bnds)
      {
        double propCenter[3] = {
          0.5*(bnds[1] + bnds[0]),
          0.5*(bnds[3] + bnds[2]),
          0.5*(bnds[5] + bnds[4])};
        double propExtent[3] = {
          (bnds[1] - bnds[0]),
          (bnds[3] - bnds[2]),
          (bnds[5] - bnds[4])};

        double length = propExtent[0] > propExtent[2] ?
          0.5 * propExtent[0] : 0.5 * propExtent[2];

        double camToPropDistance =
          sqrt(vtkMath::Distance2BetweenPoints(pos, propCenter)) - length;

        if (camToPropDistance < frameDistance)
        {
          frameDistance = camToPropDistance;
        }
      }
    }

    //Clamp distance value
    if (frameDistance < minFrameDistance)
    {
      frameDistance = minFrameDistance;
    }

    //Frame position
    double frameCenter[3];
    frameCenter[0] = pos[0] + frameDistance * dop[0];
    frameCenter[1] = pos[1] + frameDistance * dop[1];
    frameCenter[2] = pos[2] + frameDistance * dop[2];

    //Frame main directions
    double frameUp[3] = {0, 1, 0};
    double frameRight[3];
    vtkMath::Cross(dop, frameUp, frameRight);
    vtkMath::Normalize(frameRight);

    double * ori = cam->GetOrientation();
    ori[0] *= 0;
    ori[1] *= -1;
    ori[2] *= 0;

    //Update Text
    this->TextActor->SetOrigin(
      0,
      -textSize[1] + 0.5 * (frameSize[1] - offset),
      0);

    double textPosition[3];
    textPosition[0] = frameCenter[0] - (frameSize[0] - offset) * 0.5 * frameRight[0];
    textPosition[1] = frameCenter[1] - (frameSize[0] - offset) * 0.5 * frameRight[1];
    textPosition[2] = frameCenter[2] - (frameSize[0] - offset) * 0.5 * frameRight[2];

    this->TextActor->SetPosition(textPosition);
    this->TextActor->SetScale(
      FontSizeFactor*this->GetPhysicalScale(),
      FontSizeFactor*this->GetPhysicalScale(),
      1.0);
    this->TextActor->SetOrientation(ori);

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOpenVRPanelRepresentation::SetText(vtkStdString* _text)
{
  if (this->Text == *_text)
  {
    return;
  }
  this->Text = *_text;

  this->TextActor->SetInput(this->Text);
  this->Modified();
}
