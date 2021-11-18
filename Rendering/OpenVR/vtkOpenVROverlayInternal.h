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

class vtkOpenVRCameraPose : public vtkVRCamera::Pose
{
public:
  bool Loaded = false;

  void Set(vtkOpenVRCamera* cam, vtkOpenVRRenderWindow* win)
  {
    cam->SetPoseFromCamera(this, win);
    this->Loaded = true;
  }

  void Apply(vtkOpenVRCamera* cam, vtkOpenVRRenderWindow* win)
  {
    cam->ApplyPoseToCamera(this, win);
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
