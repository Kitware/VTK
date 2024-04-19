// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLBillboardTextActor3D.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkRenderer.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLBillboardTextActor3D);

//------------------------------------------------------------------------------
void vtkOpenGLBillboardTextActor3D::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkOpenGLBillboardTextActor3D::RenderTranslucentPolygonalGeometry(vtkViewport* vp)
{
  vtkOpenGLGL2PSHelper* gl2ps = vtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case vtkOpenGLGL2PSHelper::Capture: // Render to GL2PS
        return this->RenderGL2PS(vp, gl2ps);
      case vtkOpenGLGL2PSHelper::Background: // No rendering
        return 0;
      case vtkOpenGLGL2PSHelper::Inactive: // Superclass render
        break;
    }
  }

  return this->Superclass::RenderTranslucentPolygonalGeometry(vp);
}

//------------------------------------------------------------------------------
vtkOpenGLBillboardTextActor3D::vtkOpenGLBillboardTextActor3D() = default;

//------------------------------------------------------------------------------
vtkOpenGLBillboardTextActor3D::~vtkOpenGLBillboardTextActor3D() = default;

//------------------------------------------------------------------------------
int vtkOpenGLBillboardTextActor3D::RenderGL2PS(vtkViewport* viewport, vtkOpenGLGL2PSHelper* gl2ps)
{
  if (!this->InputIsValid() || !this->IsValid())
  {
    return 0;
  }

  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    vtkWarningMacro("Viewport is not a renderer?");
    return 0;
  }

  gl2ps->DrawString(this->Input, this->TextProperty, this->AnchorDC, this->AnchorDC[2] + 1e-6, ren);

  return 1;
}
VTK_ABI_NAMESPACE_END
