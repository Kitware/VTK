// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStencilMaskPass.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStencilMaskPass);

vtkInformationKeyMacro(vtkStencilMaskPass, GLStencilWrite, Integer);

//------------------------------------------------------------------------------
vtkStencilMaskPass::~vtkStencilMaskPass() = default;

//------------------------------------------------------------------------------
void vtkStencilMaskPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "AlreadyWarnedAboutStencils: " << (this->AlreadyWarnedAboutStencils ? "Yes" : "No") << "\n";
}

//------------------------------------------------------------------------------
void vtkStencilMaskPass::Render(const vtkRenderState* state)
{
  assert("pre: s_exists" && state != nullptr);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* ren = state->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());

  if (!renWin)
  {
    vtkErrorMacro("No OpenGL render window.");
    return;
  }

  // At this stage, we can't force StencilCapable to true,
  // it is already too late in the rendering pipeline so we warn the user.
  if (renWin && renWin->GetStencilCapable() == 0)
  {
    if (!this->AlreadyWarnedAboutStencils)
    {
      vtkWarningMacro(<< "Stenciling is not enabled in the render window. "
                         "The render pass will not have any effect. To fix this, "
                         "call vtkRenderWindow::StencilCapableOn().");
      this->AlreadyWarnedAboutStencils = true;
    }
    return;
  }

  vtkOpenGLState* ostate = renWin->GetState();

  vtkFrameBufferObjectBase* fbo = state->GetFrameBuffer();
  int viewportX = 0;
  int viewportY = 0;
  int viewportWidth, viewportHeight;

  if (fbo)
  {
    fbo->GetLastSize(viewportWidth, viewportHeight);
  }
  else
  {
    ren->GetTiledSizeAndOrigin(&viewportWidth, &viewportHeight, &viewportX, &viewportY);
  }

  // Create a scope to save the current color mask and depth mask state
  {
    vtkOpenGLState::ScopedglColorMask colorMaskSaver(ostate);
    vtkOpenGLState::ScopedglDepthMask depthMaskSaver(ostate);

    ostate->vtkglViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    ostate->vtkglScissor(viewportX, viewportY, viewportWidth, viewportHeight);

    ostate->vtkglEnable(GL_STENCIL_TEST);
    ostate->vtkglStencilMask(0xFF);
    glClearStencil(0);
    ostate->vtkglClear(GL_STENCIL_BUFFER_BIT);
    ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    // Replace the stencil value on a successful depth test,
    // but preserve the existing stencil value if the depth test fails
    ostate->vtkglStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);

    for (int i = 0; i < state->GetPropArrayCount(); i++)
    {
      ostate->vtkglStencilFunc(GL_ALWAYS, 0, 0xFF);
      vtkProp* p = state->GetPropArray()[i];
      vtkInformation* info = p->GetPropertyKeys();

      if (info && info->Has(vtkStencilMaskPass::GLStencilWrite()))
      {
        int stencilWrite = info->Get(vtkStencilMaskPass::GLStencilWrite());
        // Override the behavior for flagged actors
        ostate->vtkglStencilFunc(GL_ALWAYS, stencilWrite, 0xFF);
      }
      // Props having the required key fill the stencil with 1, while others erase (with 0) it if
      // the depth test pass
      int rendered = p->RenderOpaqueGeometry(state->GetRenderer());
      this->NumberOfRenderedProps += rendered;
    }
  }

  // Setup GL to only draw in unstenciled regions:
  ostate->vtkglStencilMask(0x00);
  ostate->vtkglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  ostate->vtkglStencilFunc(GL_EQUAL, 0, 0xFF);
}

VTK_ABI_NAMESPACE_END
