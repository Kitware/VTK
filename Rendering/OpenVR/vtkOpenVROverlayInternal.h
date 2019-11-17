/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPVOpenVROverlayInternal_h
#define vtkPVOpenVROverlayInternal_h

#include "vtkInteractorStyle3D.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVectorOperators.h"

class vtkOpenVRCameraPose
{
public:
  double Position[3];
  double PhysicalViewUp[3];
  double PhysicalViewDirection[3];
  double ViewDirection[3];
  double Translation[3];
  double Distance;
  double MotionFactor = 1.0;
  bool Loaded = false;

  // return a vector based on in that is orthogonal to normal
  vtkVector3d SanitizeVector(vtkVector3d& in, vtkVector3d& normal)
  {
    vtkVector3d result;
    if (fabs(in.Dot(normal)) > 0.999) // some epsilon
    {
      if (fabs(normal[0]) < 0.1)
      {
        result.Set(1.0, 0.0, 0.0);
      }
      else
      {
        result.Set(0.0, 1.0, 0.0);
      }
    }
    else
    {
      result = in - (in.Dot(normal)) * normal;
      result.Normalize();
    }
    return result;
  }

  void Set(vtkOpenVRCamera* cam, vtkOpenVRRenderWindow* win)
  {
    win->GetPhysicalTranslation(this->Translation);
    win->GetPhysicalViewUp(this->PhysicalViewUp);
    this->Distance = win->GetPhysicalScale();
    vtkInteractorStyle3D* is =
      static_cast<vtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
    this->MotionFactor = is->GetDollyPhysicalSpeed();

    cam->GetPosition(this->Position);

    win->GetPhysicalViewDirection(this->PhysicalViewDirection);
    cam->GetDirectionOfProjection(this->ViewDirection);

    this->Loaded = true;
  }

  void Apply(vtkOpenVRCamera* cam, vtkOpenVRRenderWindow* win)
  {

    // s = saved values
    vtkVector3d svup(this->PhysicalViewUp);
    vtkVector3d svdir(this->ViewDirection);
    vtkVector3d strans(this->Translation);
    vtkVector3d spos(this->Position);
    double sdistance = this->Distance;

    // c = current values
    vtkVector3d cvup;
    win->GetPhysicalViewUp(cvup.GetData());
    vtkVector3d cpos;
    cam->GetPosition(cpos.GetData());
    vtkVector3d ctrans;
    win->GetPhysicalTranslation(ctrans.GetData());
    vtkVector3d cvdir;
    cam->GetDirectionOfProjection(cvdir.GetData());
    vtkVector3d civdir;
    win->GetPhysicalViewDirection(civdir.GetData());
    double cdistance = win->GetPhysicalScale();

    // n = new values
    vtkVector3d nvup = svup;
    win->SetPhysicalViewUp(nvup.GetData());

    // sanitize the svdir, must be orthogonal to nvup
    svdir = this->SanitizeVector(svdir, nvup);

    // make sure cvdir and civdir are orthogonal to our nvup
    cvdir = this->SanitizeVector(cvdir, nvup);
    civdir = this->SanitizeVector(civdir, nvup);
    vtkVector3d civright = civdir.Cross(nvup);

    // find the new initialvdir
    vtkVector3d nivdir;
    double theta = acos(svdir.Dot(cvdir));
    if (nvup.Dot(cvdir.Cross(svdir)) < 0.0)
    {
      theta = -theta;
    }
    // rotate civdir by theta
    nivdir = civdir * cos(theta) - civright * sin(theta);
    win->SetPhysicalViewDirection(nivdir.GetData());
    vtkVector3d nivright = nivdir.Cross(nvup);

    // adjust translation so that we are in the same spot
    // as when the camera was saved
    vtkVector3d ntrans;
    vtkVector3d cppwc;
    cppwc = cpos + ctrans;
    double x = cppwc.Dot(civdir) / cdistance;
    double y = cppwc.Dot(civright) / cdistance;

    ntrans = strans * nvup + nivdir * (x * sdistance - spos.Dot(nivdir)) +
      nivright * (y * sdistance - spos.Dot(nivright));

    win->SetPhysicalTranslation(ntrans.GetData());
    cam->SetPosition(cpos.GetData());

    // this really only sets the distance as the render loop
    // sets focal point and position every frame
    vtkVector3d nfp;
    nfp = cpos + nivdir * sdistance;
    cam->SetFocalPoint(nfp.GetData());
    win->SetPhysicalScale(sdistance);

#if 0
    win->SetPhysicalViewDirection(this->PhysicalViewDirection);
    cam->SetTranslation(this->Translation);
    cam->SetFocalPoint(this->FocalPoint);
    cam->SetPosition(
      this->FocalPoint[0] - this->PhysicalViewDirection[0]*this->Distance,
      this->FocalPoint[1] - this->PhysicalViewDirection[1]*this->Distance,
      this->FocalPoint[2] - this->PhysicalViewDirection[2]*this->Distance);
#endif

    win->SetPhysicalViewUp(this->PhysicalViewUp);
    vtkInteractorStyle3D* is =
      static_cast<vtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
    is->SetDollyPhysicalSpeed(this->MotionFactor);
  }
};

class vtkOpenVROverlaySpot
{
public:
  vtkOpenVROverlaySpot(int x1, int x2, int y1, int y2, vtkCommand* cb)
  {
    this->xmin = x1;
    this->xmax = x2;
    this->ymin = y1;
    this->ymax = y2;
    this->Callback = cb;
    cb->Register(nullptr);
    this->Active = false;
  }
  ~vtkOpenVROverlaySpot()
  {
    if (this->Callback)
    {
      this->Callback->Delete();
      this->Callback = nullptr;
    }
  }
  bool Active;
  int xmin;
  int xmax;
  int ymin;
  int ymax;
  vtkCommand* Callback;
  std::string Group;
  int GroupId;

  vtkOpenVROverlaySpot(const vtkOpenVROverlaySpot& in)
  {
    this->xmin = in.xmin;
    this->xmax = in.xmax;
    this->ymin = in.ymin;
    this->ymax = in.ymax;
    this->Callback = in.Callback;
    this->Callback->Register(0);
    this->Active = in.Active;
    this->Group = in.Group;
    this->GroupId = in.GroupId;
  }
  vtkOpenVROverlaySpot& operator=(const vtkOpenVROverlaySpot&) = delete;
};

#endif // vtkPVOpenVROverlayInternal_h

//****************************************************************************
// VTK-HeaderTest-Exclude: vtkOpenVROverlayInternal.h
