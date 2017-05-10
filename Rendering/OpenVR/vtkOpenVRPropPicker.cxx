/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPropPicker.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPropPicker.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkBox.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor3D.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkOpenVRPropPicker);

vtkOpenVRPropPicker::vtkOpenVRPropPicker()
{}

vtkOpenVRPropPicker::~vtkOpenVRPropPicker()
{}

// set up for a pick
void vtkOpenVRPropPicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int vtkOpenVRPropPicker::Pick(double selectionX, double selectionY,
  double selectionZ, vtkRenderer *renderer)
{
  double selectionPt[3] = {selectionX, selectionY, selectionZ};
  //Compute event orientation
  vtkRenderWindowInteractor3D* iren = vtkRenderWindowInteractor3D::SafeDownCast(
    renderer->GetRenderWindow()->GetInteractor());
  if (!iren)
  {
    vtkErrorMacro(<< "Couldn't get 3D interactor");
    return 0;
  }
  double* wori = iren->GetWorldEventOrientation(iren->GetPointerIndex());

  if (this->PickFromList)
  {
    return this->PickProp(selectionPt, wori, renderer,
      this->PickList);
  }
  else
  {
    return this->PickProp(selectionPt, wori, renderer,
      renderer->GetViewProps());
  }
}


// Pick from the given collection
int vtkOpenVRPropPicker::PickProp(
  double selectionPt[3], double wori[4],
  vtkRenderer *renderer, vtkPropCollection* propCollection)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, NULL);

  //Event position - Ray start position
  double p0[4];
  p0[0] = selectionPt[0];
  p0[1] = selectionPt[1];
  p0[2] = selectionPt[2];
  p0[3] = 1.0;

  //Compute ray direction
  vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
  trans->RotateWXYZ(wori[0], wori[1], wori[2], wori[3]);
  double* rayDirection = trans->TransformDoubleVector(0.0, 0.0, -1.0);

  vtkCamera* cam = renderer->GetActiveCamera();
  if (!cam)
  {
    return 0;
  }
  //Ray length
  double rayLength = cam->GetClippingRange()[1];

  //Ray end point
  double p1[4];
  p1[0] = p0[0] + rayLength * rayDirection[0];
  p1[1] = p0[1] + rayLength * rayDirection[1];
  p1[2] = p0[2] + rayLength * rayDirection[2];
  p1[3] = 1.0;

  //Construct the ray
  double ray[3];
  ray[0] = p1[0] - p0[0];
  ray[1] = p1[1] - p0[1];
  ray[2] = p1[2] - p0[2];

  vtkAssemblyPath *result = NULL;
  vtkAssemblyPath *insideResult = NULL;
  vtkCollectionSimpleIterator pit;
  vtkProp *prop = NULL;
  vtkAssemblyPath *path;
  vtkProp *propCandidate;
  double t_min = VTK_DOUBLE_MAX;
  double hitPos[3];

  //For all props, return the closest prop intersected by the ray.
  //If we pick inside a prop, it will be returned only if no other vtkProps are
  //intersected by the ray. WARNING: Intersection checking uses bounds. This is
  //confusing when the prop isn't fully filling its bounds. Improve this by :
  //-returning the prop wich bounds center is the closest to the ray, or
  //-computing intersection with the geometry itself (see vtkCellPicker).
  for (propCollection->InitTraversal(pit);
    (prop = propCollection->GetNextProp(pit));)
  {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
    {
      propCandidate = path->GetFirstNode()->GetViewProp();
      if (propCandidate->GetPickable() && propCandidate->GetVisibility()
        && propCandidate->GetUseBounds())
      {
        double *bnds = propCandidate->GetBounds();
        if (bnds)
        {
          double t;
          double xyz[3];
          //Check for box intersection
          if (vtkBox::IntersectBox(bnds, const_cast<double*>(p0), ray, xyz, t))
          {
            //Inside a prop, save its path in case nothing else is picked
            if (!(t > 0))
            {
              insideResult = path;

              hitPos[0] = selectionPt[0];
              hitPos[1] = selectionPt[1];
              hitPos[2] = selectionPt[2];
            }
            //Something was picked by the ray, save its path and update t_min
            if (t > 0 && t < t_min)
            {
              result = path;
              t_min = t;
              hitPos[0] = xyz[0];
              hitPos[1] = xyz[1];
              hitPos[2] = xyz[2];
            }
          }
        }
      }
    }
  }

  // If the ray didn't intersect anything, we might be inside a prop
  if (!result)
  {
    result = insideResult;
  }

  // If something was picked..
  if (result)
  {
    result->GetFirstNode()->GetViewProp()->Pick();
    this->InvokeEvent(vtkCommand::PickEvent, NULL);

    //Update the picked position
    this->PickPosition[0] = hitPos[0];
    this->PickPosition[1] = hitPos[1];
    this->PickPosition[2] = hitPos[2];
  }

  this->SetPath(result);
  this->InvokeEvent(vtkCommand::EndPickEvent, NULL);

  // Call Pick on the Prop that was picked, and return 1 for success
  if (result)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void vtkOpenVRPropPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
