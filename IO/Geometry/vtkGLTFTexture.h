// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkGLTFTexture_h
#define vtkGLTFTexture_h

#include "GLTFSampler.h"         // For Sampler
#include "vtkIOGeometryModule.h" // For export macro
#include "vtkObjectBase.h"
#include "vtkSetGet.h"       // For vtkBaseTypeMacro
#include "vtkSmartPointer.h" // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkTexture;

class VTKIOGEOMETRY_EXPORT vtkGLTFTexture : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkGLTFTexture, vtkObjectBase);
  static vtkGLTFTexture* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
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
