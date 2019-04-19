#include "vtkOpenGLFXAAPass.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"

vtkStandardNewMacro(vtkOpenGLFXAAPass);

// ----------------------------------------------------------------------------
void vtkOpenGLFXAAPass::Render(const vtkRenderState* s)
{
  vtkOpenGLRenderer* r = vtkOpenGLRenderer::SafeDownCast(s->GetRenderer());
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  ostate->vtkglViewport(x, y, w, h);
  ostate->vtkglScissor(x, y, w, h);

  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro("no delegate in vtkOpenGLFXAAPass.");
    return;
  }

  this->DelegatePass->Render(s);
  this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();

  if (r->GetFXAAOptions())
  {
    this->FXAAFilter->UpdateConfiguration(r->GetFXAAOptions());
  }

  this->FXAAFilter->Execute(r);
}

// ----------------------------------------------------------------------------
void vtkOpenGLFXAAPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
