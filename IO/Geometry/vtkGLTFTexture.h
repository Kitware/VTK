// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkGLTFTexture_h
#define vtkGLTFTexture_h

#include "GLTFSampler.h"
#include "vtkIOGeometryModule.h" // For export macro
#include "vtkObjectBase.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkTexture;

struct VTKIOGEOMETRY_EXPORT vtkGLTFTexture : public vtkObjectBase
{
  vtkBaseTypeMacro(vtkGLTFTexture, vtkObjectBase);
  static vtkGLTFTexture* New();
  vtkSmartPointer<vtkImageData> Image;
  GLTFSampler Sampler;
  vtkSmartPointer<vtkTexture> GetVTKTexture();

protected:
  vtkGLTFTexture() = default;
  ~vtkGLTFTexture() override = default;

private:
  vtkGLTFTexture(const vtkGLTFTexture&) = delete;
  void operator=(const vtkGLTFTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
