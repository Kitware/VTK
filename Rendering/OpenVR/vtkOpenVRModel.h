/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkOpenVRModel
* @brief   OpenVR device model

* This internal class is used to load models
* such as for the trackers and controllers and to
* render them in the scene
*/

#ifndef vtkOpenVRModel_h
#define vtkOpenVRModel_h

#include "vtkNew.h" // for ivar
#include "vtkObject.h"
#include "vtkOpenGLHelper.h"          // ivar
#include "vtkRenderingOpenVRModule.h" // For export macro
#include <openvr.h>                   // for ivars

class vtkOpenVRRenderWindow;
class vtkRenderWindow;
class vtkOpenGLVertexBufferObject;
class vtkTextureObject;
class vtkMatrix4x4;
class vtkOpenVRRay;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRModel : public vtkObject
{
public:
  static vtkOpenVRModel* New();
  vtkTypeMacro(vtkOpenVRModel, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Build(vtkOpenVRRenderWindow* win);
  void Render(vtkOpenVRRenderWindow* win, const vr::TrackedDevicePose_t& pose);

  const std::string& GetName() const { return this->ModelName; }
  void SetName(const std::string& modelName) { this->ModelName = modelName; }

  // show the model
  void SetVisibility(bool v) { this->Visibility = v; }
  bool GetVisibility() { return this->Visibility; }

  // Set Ray parameters
  void SetShowRay(bool v);
  void SetRayLength(double length);
  void SetRayColor(double r, double g, double b);
  vtkOpenVRRay* GetRay() { return this->Ray; }

  void ReleaseGraphicsResources(vtkWindow* win);

  // the tracked device this model represents if any
  vr::TrackedDeviceIndex_t TrackedDevice;

  vr::RenderModel_t* RawModel;

protected:
  vtkOpenVRModel();
  ~vtkOpenVRModel() override;

  std::string ModelName;

  bool Visibility;
  bool Loaded;
  bool FailedToLoad;

  vr::RenderModel_TextureMap_t* RawTexture;
  vtkOpenGLHelper ModelHelper;
  vtkOpenGLVertexBufferObject* ModelVBO;
  vtkNew<vtkTextureObject> TextureObject;
  vtkNew<vtkMatrix4x4> PoseMatrix;

  // Controller ray
  vtkNew<vtkOpenVRRay> Ray;

private:
  vtkOpenVRModel(const vtkOpenVRModel&) = delete;
  void operator=(const vtkOpenVRModel&) = delete;
};

#endif
