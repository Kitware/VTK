// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkPVOpenVROverlayInternal_h
#define vtkPVOpenVROverlayInternal_h

#include "vtkInteractorStyle3D.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVectorOperators.h"

VTK_ABI_NAMESPACE_BEGIN
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
    this->Callback->Register(nullptr);
    this->Active = in.Active;
    this->Group = in.Group;
    this->GroupId = in.GroupId;
  }
  vtkOpenVROverlaySpot& operator=(const vtkOpenVROverlaySpot&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPVOpenVROverlayInternal_h

//****************************************************************************
// VTK-HeaderTest-Exclude: vtkOpenVROverlayInternal.h
