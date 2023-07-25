// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkVRModel.h"
#include <openvr.h> // for ivars

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENVR_EXPORT vtkOpenVRModel : public vtkVRModel
{
public:
  static vtkOpenVRModel* New();
  vtkTypeMacro(vtkOpenVRModel, vtkVRModel);

  vr::RenderModel_t* RawModel;

protected:
  vtkOpenVRModel();
  ~vtkOpenVRModel() override = default;

  void FillModelHelper() override;
  void SetPositionAndTCoords() override;
  void CreateTextureObject(vtkOpenGLRenderWindow* win) override;
  void LoadModelAndTexture(vtkOpenGLRenderWindow* win) override;
  vr::RenderModel_TextureMap_t* RawTexture;

private:
  vtkOpenVRModel(const vtkOpenVRModel&) = delete;
  void operator=(const vtkOpenVRModel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
