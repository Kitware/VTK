// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "ExternalVTKWidget.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(ExternalVTKWidget);

//------------------------------------------------------------------------------
ExternalVTKWidget::ExternalVTKWidget()
{
  this->RenderWindow = nullptr;
}

//------------------------------------------------------------------------------
ExternalVTKWidget::~ExternalVTKWidget()
{
  this->SetRenderWindow(nullptr);
}

//------------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow* ExternalVTKWidget::GetRenderWindow()
{
  if (!this->RenderWindow)
  {
    vtkExternalOpenGLRenderWindow* win = vtkExternalOpenGLRenderWindow::New();
    this->SetRenderWindow(win);
    win->Delete();
  }
  return this->RenderWindow;
}

//------------------------------------------------------------------------------
vtkExternalOpenGLRenderer* ExternalVTKWidget::AddRenderer()
{
  vtkExternalOpenGLRenderer* ren = vtkExternalOpenGLRenderer::New();
  this->GetRenderWindow()->AddRenderer(ren);
  ren->Delete();
  return ren;
}

//------------------------------------------------------------------------------
void ExternalVTKWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void ExternalVTKWidget::SetRenderWindow(vtkExternalOpenGLRenderWindow* w)
{
  // Do nothing if we don't have to
  if (this->RenderWindow == w)
  {
    return;
  }

  // Swap the renderers from the old window to the new one
  vtkRendererCollection* renderers = nullptr;

  // Unregister the previous window
  if (this->RenderWindow)
  {
    renderers = this->RenderWindow->GetRenderers();
    // Increase reference count of the renderer collection to
    // make sure the reference exists for the new render window
    renderers->Register(this);
    this->RenderWindow->Finalize();
    this->RenderWindow->SetMapped(0);
    this->RenderWindow->UnRegister(this);
  }

  // Now, set the new window
  this->RenderWindow = w;

  if (this->RenderWindow)
  {
    // If it is mapped somewhere else, unmap it.
    this->RenderWindow->Finalize();
    this->RenderWindow->SetMapped(1);
    this->RenderWindow->Register(this);

    if (renderers)
    {
      // Add the renderers
      vtkRenderer* aren;
      vtkCollectionSimpleIterator rsit;

      for (renderers->InitTraversal(rsit); (aren = renderers->GetNextRenderer(rsit));)
      {
        this->RenderWindow->AddRenderer(aren);
      }
    }
  }
  if (renderers)
  {
    // Decrease reference count of the renderer collection
    renderers->UnRegister(this);
  }
}
VTK_ABI_NAMESPACE_END
