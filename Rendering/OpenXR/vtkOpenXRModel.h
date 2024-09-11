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

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRModel : public vtkVRModel
{
public:
  static vtkOpenXRModel* New();
  vtkTypeMacro(vtkOpenXRModel, vtkVRModel);

  void SetAssetPath(const std::string& assetPath) { this->AssetPath = assetPath; }

  const std::string& GetAssetPath() { return this->AssetPath; }

protected:
  vtkOpenXRModel();
  ~vtkOpenXRModel() override;

  void FillModelHelper() override;
  void SetPositionAndTCoords() override;
  void CreateTextureObject(vtkOpenGLRenderWindow* win) override;
  void LoadModelAndTexture(vtkOpenGLRenderWindow* win) override;

  std::string AssetPath;

private:
  vtkOpenXRModel(const vtkOpenXRModel&) = delete;
  void operator=(const vtkOpenXRModel&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;
};

VTK_ABI_NAMESPACE_END
#endif
