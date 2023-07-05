// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
* @class   vtkOpenXRModel
* @brief   OpenXR device model

* This internal class is used to load models
* such as for the trackers and controllers and to
* render them in the scene
*/

#ifndef vtkOpenXRModel_h
#define vtkOpenXRModel_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRModel.h"
#include <atomic> // for ivars
#include <vector> // for ivars

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRModel : public vtkVRModel
{
public:
  static vtkOpenXRModel* New();
  vtkTypeMacro(vtkOpenXRModel, vtkVRModel);

protected:
  vtkOpenXRModel() = default;
  ~vtkOpenXRModel() override = default;

  void FillModelHelper() override;
  void SetPositionAndTCoords() override;
  void CreateTextureObject(vtkOpenGLRenderWindow* win) override;
  void LoadModelAndTexture(vtkOpenGLRenderWindow* win) override;

  std::atomic<bool> ModelLoading{ false };
  std::atomic<bool> ModelLoaded{ false };
  void AsyncLoad();

  std::vector<float> ModelVBOData;
  std::vector<uint16_t> ModelIBOData;
  std::vector<uint8_t> TextureData;

private:
  vtkOpenXRModel(const vtkOpenXRModel&) = delete;
  void operator=(const vtkOpenXRModel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
