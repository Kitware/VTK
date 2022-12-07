/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Corporation from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/openvr/blob/master/LICENSE

=========================================================================*/
#include "vtkVRRenderer.h"

#include "vtkCamera.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkVRRenderWindow.h"

//------------------------------------------------------------------------------
vtkVRRenderer::vtkVRRenderer()
{
  this->FloorActor->PickableOff();

  vtkNew<vtkPolyDataMapper> pdm;
  this->FloorActor->SetMapper(pdm);
  vtkNew<vtkPlaneSource> plane;
  pdm->SetInputConnection(plane->GetOutputPort());
  plane->SetOrigin(-5.0, 0.0, -5.0);
  plane->SetPoint1(5.0, 0.0, -5.0);
  plane->SetPoint2(-5.0, 0.0, 5.0);

  vtkNew<vtkTransform> tf;
  tf->Identity();
  this->FloorActor->SetUserTransform(tf);

  vtkNew<vtkTexture> texture;
  this->FloorActor->SetTexture(texture);

  // build a grid fading off in the distance
  vtkNew<vtkImageCanvasSource2D> grid;
  grid->SetScalarTypeToUnsignedChar();
  grid->SetNumberOfScalarComponents(4);
  grid->SetExtent(0, 511, 0, 511, 0, 0);
  int divisions = 16;
  int divSize = 512 / divisions;
  double alpha = 1.0;
  for (int i = 0; i < divisions; i++)
  {
    for (int j = 0; j < divisions; j++)
    {
      grid->SetDrawColor(255, 255, 255, 255 * alpha);
      grid->FillBox(i * divSize, (i + 1) * divSize - 1, j * divSize, (j + 1) * divSize - 1);
      grid->SetDrawColor(230, 230, 230, 255 * alpha);
      grid->DrawSegment(i * divSize, j * divSize, (i + 1) * divSize - 1, j * divSize);
      grid->DrawSegment(i * divSize, j * divSize, i * divSize, (j + 1) * divSize - 1);
    }
  }

  texture->SetInputConnection(grid->GetOutputPort());

  this->FloorActor->SetUseBounds(false);
  this->ShowFloor = false;
}

//------------------------------------------------------------------------------
void vtkVRRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ShowFloor " << (this->ShowFloor ? "On\n" : "Off\n");
  this->FloorActor->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// adjust the floor if we need to
void vtkVRRenderer::DeviceRender()
{
  if (this->ShowFloor)
  {
    vtkNew<vtkTransform> floorTransform;
    this->GetFloorTransform(floorTransform);
    this->FloorActor->SetUserTransform(floorTransform);
  }
  this->Superclass::DeviceRender();
}

//------------------------------------------------------------------------------
void vtkVRRenderer::GetFloorTransform(vtkTransform* transform)
{
  vtkVRRenderWindow* win = static_cast<vtkVRRenderWindow*>(this->GetRenderWindow());

  double physicalScale = win->GetPhysicalScale();

  double trans[3];
  win->GetPhysicalTranslation(trans);

  double* vup = win->GetPhysicalViewUp();
  double* dop = win->GetPhysicalViewDirection();
  double vr[3];
  vtkMath::Cross(dop, vup, vr);
  double rot[16] = { vr[0], vup[0], -dop[0], 0.0, vr[1], vup[1], -dop[1], 0.0, vr[2], vup[2],
    -dop[2], 0.0, 0.0, 0.0, 0.0, 1.0 };

  transform->Identity();
  transform->Translate(-trans[0], -trans[1], -trans[2]);
  transform->Scale(physicalScale, physicalScale, physicalScale);
  transform->Concatenate(rot);
}

//------------------------------------------------------------------------------
void vtkVRRenderer::SetShowFloor(bool value)
{
  if (this->ShowFloor == value)
  {
    return;
  }

  this->ShowFloor = value;

  if (this->ShowFloor)
  {
    this->AddActor(this->FloorActor);
  }
  else
  {
    this->RemoveActor(this->FloorActor);
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderer::ResetCamera(const double bounds[6])
{
  double center[3];
  double distance;
  double vn[3], *vup;

  this->GetActiveCamera();
  if (this->ActiveCamera != nullptr)
  {
    this->ActiveCamera->GetViewPlaneNormal(vn);
  }
  else
  {
    vtkErrorMacro(<< "Trying to reset non-existent camera");
    return;
  }

  // Reset the perspective zoom factors, otherwise subsequent zooms will cause
  // the view angle to become very small and cause bad depth sorting.
  this->ActiveCamera->SetViewAngle(110.0);

  double expandedBounds[6] = { bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5] };
  this->ExpandBounds(expandedBounds, this->ActiveCamera->GetModelTransformMatrix());

  center[0] = (expandedBounds[0] + expandedBounds[1]) / 2.0;
  center[1] = (expandedBounds[2] + expandedBounds[3]) / 2.0;
  center[2] = (expandedBounds[4] + expandedBounds[5]) / 2.0;

  double w1 = expandedBounds[1] - expandedBounds[0];
  double w2 = expandedBounds[3] - expandedBounds[2];
  double w3 = expandedBounds[5] - expandedBounds[4];
  w1 *= w1;
  w2 *= w2;
  w3 *= w3;
  double radius = w1 + w2 + w3;

  // If we have just a single point, pick a radius of 1.0
  radius = (radius == 0) ? (1.0) : (radius);

  // compute the radius of the enclosing sphere
  radius = sqrt(radius) * 0.5;

  // default so that the bounding sphere fits within the view fustrum

  // compute the distance from the intersection of the view frustum with the
  // bounding sphere. Basically in 2D draw a circle representing the bounding
  // sphere in 2D then draw a horizontal line going out from the center of
  // the circle. That is the camera view. Then draw a line from the camera
  // position to the point where it intersects the circle. (it will be tangent
  // to the circle at this point, this is important, only go to the tangent
  // point, do not draw all the way to the view plane). Then draw the radius
  // from the tangent point to the center of the circle. You will note that
  // this forms a right triangle with one side being the radius, another being
  // the target distance for the camera, then just find the target dist using
  // a sin.
  double angle = vtkMath::RadiansFromDegrees(this->ActiveCamera->GetViewAngle());

  this->ComputeAspect();
  double aspect[2];
  this->GetAspect(aspect);

  if (aspect[0] >= 1.0) // horizontal window, deal with vertical angle|scale
  {
    if (this->ActiveCamera->GetUseHorizontalViewAngle())
    {
      angle = 2.0 * atan(tan(angle * 0.5) / aspect[0]);
    }
  }
  else // vertical window, deal with horizontal angle|scale
  {
    if (!this->ActiveCamera->GetUseHorizontalViewAngle())
    {
      angle = 2.0 * atan(tan(angle * 0.5) * aspect[0]);
    }
  }
  distance = radius / sin(angle * 0.5);

  // check view-up vector against view plane normal
  vup = this->ActiveCamera->GetViewUp();
  if (fabs(vtkMath::Dot(vup, vn)) > 0.999)
  {
    vtkWarningMacro(<< "Resetting view-up since view plane normal is parallel");
    this->ActiveCamera->SetViewUp(-vup[2], vup[0], vup[1]);
  }

  // update the camera
  this->ActiveCamera->SetFocalPoint(center[0], center[1], center[2]);
  this->ActiveCamera->SetPosition(
    center[0] + distance * vn[0], center[1] + distance * vn[1], center[2] + distance * vn[2]);

  // now set the cameras shift and scale to the HMD space
  // since the vive is always in meters (or something like that)
  // we use a shift scale to map view space into hmd view space
  // that way the solar system can be modelled in its units
  // while the shift scale maps it into meters.  This can also
  // be done in the actors but then it requires every actor
  // to be adjusted.  It cannot be done with the camera model
  // matrix as that is broken.
  // The additional distance translation in the view up direction is because we
  // want the center of the world to be above the physical floor instead of at its level.
  vtkVRRenderWindow* win = static_cast<vtkVRRenderWindow*>(this->GetRenderWindow());
  win->SetPhysicalTranslation(
    -center[0] + vup[0] * distance, -center[1] + vup[1] * distance, -center[2] + vup[2] * distance);

  win->SetPhysicalScale(distance);
}

//------------------------------------------------------------------------------
void vtkVRRenderer::ResetCameraClippingRange()
{
  double bounds[6];

  this->ComputeVisiblePropBounds(bounds);

  this->GetActiveCameraAndResetIfCreated();
  if (this->ActiveCamera == nullptr)
  {
    vtkErrorMacro(<< "Trying to reset clipping range of non-existent camera");
    return;
  }

  vtkVRRenderWindow* win = static_cast<vtkVRRenderWindow*>(this->GetRenderWindow());
  double physicalScale = win->GetPhysicalScale();

  // reset the clipping range when we don't have any 3D visible props
  if (!vtkMath::AreBoundsInitialized(bounds))
  {
    // default to 0.2 to 10.0 meters in physical space if no data bounds
    this->ActiveCamera->SetClippingRange(0.2 * physicalScale, 10.0 * physicalScale);
    return;
  }

  this->ResetCameraClippingRange(bounds);
}

//------------------------------------------------------------------------------
void vtkVRRenderer::ResetCameraClippingRange(const double bounds[6])
{
  this->GetActiveCameraAndResetIfCreated();
  if (this->ActiveCamera == nullptr)
  {
    vtkErrorMacro(<< "Trying to reset clipping range of non-existent camera");
    return;
  }

  double range[2];
  int i, j, k;
  vtkVRRenderWindow* win = static_cast<vtkVRRenderWindow*>(this->GetRenderWindow());
  double physicalScale = win->GetPhysicalScale();

  // reset the clipping range when we don't have any 3D visible props
  if (!vtkMath::AreBoundsInitialized(bounds))
  {
    // default to 0.2 to 10.0 meters in physical space if no data bounds
    this->ActiveCamera->SetClippingRange(0.2 * physicalScale, 10.0 * physicalScale);
    return;
  }

  double expandedBounds[6] = { bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5] };
  this->ExpandBounds(expandedBounds, this->ActiveCamera->GetModelTransformMatrix());

  double trans[3];
  win->GetPhysicalTranslation(trans);

  range[0] = 0.2; // 20 cm in front of HMD
  range[1] = 0.0;

  // Find the farthest bounding box vertex
  for (k = 0; k < 2; k++)
  {
    for (j = 0; j < 2; j++)
    {
      for (i = 0; i < 2; i++)
      {
        double fard = sqrt((expandedBounds[i] - trans[0]) * (expandedBounds[i] - trans[0]) +
          (expandedBounds[2 + j] - trans[1]) * (expandedBounds[2 + j] - trans[1]) +
          (expandedBounds[4 + k] - trans[2]) * (expandedBounds[4 + k] - trans[2]));
        range[1] = (fard > range[1]) ? fard : range[1];
      }
    }
  }

  range[1] /= physicalScale; // convert to physical scale
  range[1] += 3.0;           // add 3 meters for room to walk around

  // to see transmitters make sure far is at least 10 meters
  if (range[1] < 10.0)
  {
    range[1] = 10.0;
  }

  this->ActiveCamera->SetClippingRange(range[0] * physicalScale, range[1] * physicalScale);
}
