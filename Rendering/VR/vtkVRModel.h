// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRModel
 * @brief   VR device model
 *
 * Abstract class used to load models
 * such as for the trackers and controllers and to
 * render them in the scene
 *
 */

#ifndef vtkVRModel_h
#define vtkVRModel_h

#include "vtkNew.h" // for ivar
#include "vtkObject.h"
#include "vtkOpenGLHelper.h"      // ivar
#include "vtkRenderingVRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix4x4;
class vtkOpenGLRenderWindow;
class vtkOpenGLVertexBufferObject;
class vtkTextureObject;
class vtkVRRay;

class VTKRENDERINGVR_EXPORT vtkVRModel : public vtkObject
{
public:
  vtkTypeMacro(vtkVRModel, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Build(vtkOpenGLRenderWindow* win);

  void Render(vtkOpenGLRenderWindow* win, vtkMatrix4x4* poseInTrackingCoordinates);

  const std::string& GetName() const { return this->ModelName; }
  void SetName(const std::string& modelName) { this->ModelName = modelName; }

  // show the model
  void SetVisibility(bool v) { this->Visibility = v; }
  bool GetVisibility() { return this->Visibility; }

  // Set Ray parameters
  void SetShowRay(bool v);
  void SetRayLength(double length);
  void SetRayColor(double r, double g, double b);
  vtkVRRay* GetRay() { return this->Ray; }

  void ReleaseGraphicsResources(vtkWindow* win);

protected:
  vtkVRModel();
  ~vtkVRModel() override;

  virtual void FillModelHelper() = 0;
  virtual void SetPositionAndTCoords() = 0;
  virtual void CreateTextureObject(vtkOpenGLRenderWindow* win) = 0;
  virtual void LoadModelAndTexture(vtkOpenGLRenderWindow* win) = 0;

  std::string ModelName;

  bool Visibility;
  bool Loaded;
  bool FailedToLoad;

  vtkOpenGLHelper ModelHelper;
  vtkOpenGLVertexBufferObject* ModelVBO;
  vtkNew<vtkTextureObject> TextureObject;
  vtkNew<vtkMatrix4x4> ModelToProjectionMatrix;

  // Controller ray
  vtkNew<vtkVRRay> Ray;

private:
  vtkVRModel(const vtkVRModel&) = delete;
  void operator=(const vtkVRModel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
