// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLArrayTextureBufferAdapter
 * @brief   Interfaces vtkDataArray to an OpenGL texture buffer.
 *
 */

#ifndef vtkOpenGLArrayTextureBufferAdapter_h
#define vtkOpenGLArrayTextureBufferAdapter_h

#include "vtkDataArray.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkRenderingOpenGL2Module.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkWindow.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLArrayTextureBufferAdapter
{
public:
  std::vector<vtkSmartPointer<vtkDataArray>> Arrays;
  vtkSmartPointer<vtkTextureObject> Texture;
  vtkSmartPointer<vtkOpenGLBufferObject> Buffer;
  vtkOpenGLBufferObject::ObjectType BufferType;
  vtkOpenGLBufferObject::ObjectUsage BufferUsage;
  bool IntegerTexture;
  bool ScalarComponents;

  vtkOpenGLArrayTextureBufferAdapter();
  vtkOpenGLArrayTextureBufferAdapter(
    vtkDataArray* array, bool asScalars, bool* integerTexture = nullptr);
  vtkOpenGLArrayTextureBufferAdapter(const vtkOpenGLArrayTextureBufferAdapter&) = default;
  vtkOpenGLArrayTextureBufferAdapter& operator=(
    const vtkOpenGLArrayTextureBufferAdapter&) = default;

  void Upload(vtkOpenGLRenderWindow* renderWindow, bool force = false);

  void ReleaseGraphicsResources(vtkWindow* window);
};

VTK_ABI_NAMESPACE_END
#endif
// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkOpenGLArrayTextureBufferAdapter.h
