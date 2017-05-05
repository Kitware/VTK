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

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkObject.h"
#include "vtkOpenGLHelper.h"
#include "vtkNew.h"
#include <openvr.h> // for ivars

class vtkOpenVRRenderWindow;
class vtkRenderWindow;
class vtkOpenGLVertexBufferObject;
class vtkTextureObject;
class vtkMatrix4x4;
class vtkOpenVRRay;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRModel : public vtkObject
{
public:
  static vtkOpenVRModel *New();
  vtkTypeMacro(vtkOpenVRModel, vtkObject);

  bool Build(vtkOpenVRRenderWindow *win);
  void Render(vtkOpenVRRenderWindow *win,
    const vr::TrackedDevicePose_t &pose);

  const std::string & GetName() const {
    return this->ModelName;
  }
  void SetName(const std::string & modelName) {
    this->ModelName = modelName;
  };

  // show the model
  void SetShow(bool v) {
    this->Show = v;
  };
  bool GetShow() {
    return this->Show;
  };

  //Set Ray parameters
  void SetShowRay(bool v);
  void SetRayLength(double length);

  void ReleaseGraphicsResources(vtkRenderWindow *win);

  vr::RenderModel_t *RawModel;

protected:
  vtkOpenVRModel();
  ~vtkOpenVRModel();

  std::string ModelName;

  bool Show;
  bool Loaded;
  bool FailedToLoad;

  vr::RenderModel_TextureMap_t *RawTexture;
  vtkOpenGLHelper ModelHelper;
  vtkOpenGLVertexBufferObject *ModelVBO;
  vtkNew<vtkTextureObject> TextureObject;
  vtkNew<vtkMatrix4x4> PoseMatrix;

  //Controller ray
  vtkNew<vtkOpenVRRay> Ray;
};

#endif
