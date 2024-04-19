// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLowMemoryStripsAgent
 * @brief   Maps VTK_TRIANGLE_STRIP into GL_TRIANGLES and draws GL_TRIANGLES or GL_LINES or
 * GL_POINTS.
 */

#ifndef vtkOpenGLLowMemoryStripsAgent_h
#define vtkOpenGLLowMemoryStripsAgent_h

#include "vtkOpenGLLowMemoryPolygonsAgent.h"

#include "vtkRenderingOpenGL2Module.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_NO_EXPORT vtkOpenGLLowMemoryStripsAgent
  : public vtkOpenGLLowMemoryPolygonsAgent
{
public:
  vtkOpenGLLowMemoryStripsAgent();
  ~vtkOpenGLLowMemoryStripsAgent() override;
};

VTK_ABI_NAMESPACE_END
#endif

// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkOpenGLLowMemoryStripsAgent.h
