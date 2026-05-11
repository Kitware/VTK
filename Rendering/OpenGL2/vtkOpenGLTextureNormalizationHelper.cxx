// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLTextureNormalizationHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTextureCPUNormalization.h"
#include "vtkOpenGLTextureComputeShaderNormalization.h"
#include "vtkOpenGLTextureFramebufferNormalization.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
void vtkOpenGLTextureNormalizationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConversionMode: ";
  switch (this->Mode)
  {
    case ConversionMode::CPU:
      os << "CPU\n";
      break;
    case ConversionMode::ComputeShader:
      os << "ComputeShader\n";
      break;
    case ConversionMode::CopyTexImage:
      os << "CopyTexImage\n";
      break;
    case ConversionMode::Unsupported:
      os << "Unsupported\n";
      break;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkOpenGLTextureNormalizationHelper> vtkOpenGLTextureNormalizationHelper::Create(
  vtkOpenGLRenderWindow* context)
{
#ifdef GL_ES_VERSION_3_0
  if (!context)
  {
    return vtkSmartPointer<vtkOpenGLTextureCPUNormalization>::New();
  }

  context->MakeCurrent();

  // Check for compute shader support (GL 4.3+ or ARB_compute_shader)
#ifdef GL_COMPUTE_SHADER
  if (GLAD_GL_ARB_compute_shader || GLAD_GL_VERSION_4_3)
  {
    auto helper = vtkSmartPointer<vtkOpenGLTextureComputeShaderNormalization>::New();
    helper->Initialize(context);
    if (helper->GetConversionMode() != ConversionMode::Unsupported)
    {
      return helper;
    }
  }
#endif

  // Fallback to framebuffer-based conversion
  {
    auto helper = vtkSmartPointer<vtkOpenGLTextureFramebufferNormalization>::New();
    helper->Initialize(context);
    if (helper->GetConversionMode() != ConversionMode::Unsupported)
    {
      return helper;
    }
  }

  // Final fallback: CPU conversion
  return vtkSmartPointer<vtkOpenGLTextureCPUNormalization>::New();
#else
  // Desktop OpenGL: CPU conversion is sufficient
  (void)context;
  return vtkSmartPointer<vtkOpenGLTextureCPUNormalization>::New();
#endif
}

VTK_ABI_NAMESPACE_END
