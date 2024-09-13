// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

#include "vtk_glad.h"

class vtkStartRenderObserver : public vtkCommand
{
public:
  static vtkStartRenderObserver* New() { return new vtkStartRenderObserver; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event),
    void* vtkNotUsed(calldata)) override
  {
    // put something into a framebuffer
    int const* size = this->RenderWindow->GetSize();
    this->Framebuffer->Resize(size[0], size[1]);

    auto ostate = this->RenderWindow->GetState();
    ostate->PushFramebufferBindings();
    this->Framebuffer->Bind();
    this->Framebuffer->ActivateDrawBuffer(0);

    // make the left half green and initialize the
    // depth buffer to 0.7 so that some geometry gets clipped
    ostate->vtkglScissor(0, 0, size[0] / 2, size[1]);
    ostate->vtkglClearColor(0.1, 0.3, 0.2, 1.0);
    ostate->vtkglClearDepth(0.7);
    ostate->vtkglDepthMask(GL_TRUE);
    ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ostate->vtkglScissor(0, 0, size[0], size[1]);

    this->Framebuffer->ActivateReadBuffer(0);
    this->Framebuffer->DeactivateDrawBuffers();

    this->RenderWindow->BlitToRenderFramebuffer(0, 0, size[0] / 2, size[1], 0, 0, size[0] / 2,
      size[1], GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    this->Framebuffer->ActivateDrawBuffer(0);

    // make the right half blue and initialize the
    // depth buffer to 1.0
    ostate->vtkglScissor(size[0] / 2, 0, size[0] / 2, size[1]);
    ostate->vtkglClearColor(0.1, 0.2, 0.4, 1.0);
    ostate->vtkglClearDepth(1.0);
    ostate->vtkglDepthMask(GL_TRUE);
    ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ostate->vtkglScissor(0, 0, size[0], size[1]);

    this->Framebuffer->ActivateReadBuffer(0);
    this->Framebuffer->DeactivateDrawBuffers();

    this->RenderWindow->BlitToRenderFramebuffer(size[0] / 2, 0, size[0] / 2, size[1], size[0] / 2,
      0, size[0] / 2, size[1], GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    ostate->PopFramebufferBindings();
  }

  vtkOpenGLRenderWindow* RenderWindow;
  vtkOpenGLFramebufferObject* Framebuffer;

  void SetRenderWindow(vtkRenderWindow* rw)
  {
    this->RenderWindow = static_cast<vtkOpenGLRenderWindow*>(rw);
    this->Framebuffer->SetContext(this->RenderWindow);
    this->RenderWindow->GetState()->PushFramebufferBindings();
    int const* size = this->RenderWindow->GetSize();
    this->Framebuffer->PopulateFramebuffer(size[0], size[1],
      true,                 // textures
      1, VTK_UNSIGNED_CHAR, // 1 color buffer uchar
      true, 32,             // depth buffer
      0, this->RenderWindow->GetStencilCapable() != 0);
    this->RenderWindow->GetState()->PopFramebufferBindings();
  }

protected:
  vtkStartRenderObserver()
  {
    this->RenderWindow = nullptr;
    this->Framebuffer = vtkOpenGLFramebufferObject::New();
  }
  ~vtkStartRenderObserver() override { this->Framebuffer->Delete(); }
};

class vtkEndRenderObserver : public vtkCommand
{
public:
  static vtkEndRenderObserver* New() { return new vtkEndRenderObserver; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event),
    void* vtkNotUsed(calldata)) override
  {
    // put something into a framebuffer
    int const* size = this->RenderWindow->GetSize();
    int qsize[2] = { size[0] / 4, size[1] / 4 };
    this->Framebuffer->Resize(qsize[0], qsize[1]);

    auto ostate = this->RenderWindow->GetState();
    ostate->PushFramebufferBindings();
    this->Framebuffer->Bind();
    this->Framebuffer->ActivateDrawBuffer(0);

    // copy the middle of the last frame rendered
    this->RenderWindow->BlitDisplayFramebuffer(0, qsize[0], qsize[1], size[0] / 2, size[1] / 2, 0,
      0, qsize[0], qsize[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);

    this->Framebuffer->DeactivateDrawBuffers();
    this->Framebuffer->ActivateReadBuffer(0);

    // draw it in lower left corner at half size
    this->RenderWindow->BlitToRenderFramebuffer(
      0, 0, qsize[0], qsize[1], 0, 0, qsize[0], qsize[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ostate->PopFramebufferBindings();
  }

  vtkOpenGLRenderWindow* RenderWindow;
  vtkOpenGLFramebufferObject* Framebuffer;

  void SetRenderWindow(vtkRenderWindow* rw)
  {
    this->RenderWindow = static_cast<vtkOpenGLRenderWindow*>(rw);
    this->Framebuffer->SetContext(this->RenderWindow);
    this->RenderWindow->GetState()->PushFramebufferBindings();
    int const* size = this->RenderWindow->GetSize();
    this->Framebuffer->PopulateFramebuffer(size[0] / 4, size[1] / 4,
      true,                 // textures
      1, VTK_UNSIGNED_CHAR, // 1 color buffer uchar
      true, 32,             // depth buffer
      0, this->RenderWindow->GetStencilCapable() != 0);
    this->RenderWindow->GetState()->PopFramebufferBindings();
  }

protected:
  vtkEndRenderObserver()
  {
    this->RenderWindow = nullptr;
    this->Framebuffer = vtkOpenGLFramebufferObject::New();
  }
  ~vtkEndRenderObserver() override { this->Framebuffer->Delete(); }
};

//------------------------------------------------------------------------------
int TestWindowBlits(int argc, char* argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  if (!renderWindow->SupportsOpenGL())
  {
    cerr << "The platform does not support OpenGL as required\n";
    cerr << vtkOpenGLRenderWindow::SafeDownCast(renderWindow)->GetOpenGLSupportMessage();
    cerr << renderWindow->ReportCapabilities();
    return 1;
  }

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

  renderWindow->SetMultiSamples(8);

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();

  // must render once to create context etc
  renderWindow->Render();

  int major, minor;
  vtkOpenGLRenderWindow::SafeDownCast(renderWindow)->GetOpenGLVersion(major, minor);
  cerr << "opengl version " << major << "." << minor << "\n";

  vtkNew<vtkStartRenderObserver> startObserver;
  startObserver->SetRenderWindow(renderWindow);
  renderWindow->AddObserver(vtkCommand::StartEvent, startObserver);
  vtkNew<vtkEndRenderObserver> endObserver;
  endObserver->SetRenderWindow(renderWindow);
  renderWindow->AddObserver(vtkCommand::RenderEvent, endObserver);

  renderer->GetActiveCamera()->Azimuth(80);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();
  renderer->PreserveColorBufferOn();
  renderer->PreserveDepthBufferOn();
  renderer->GetActiveCamera()->Azimuth(-20);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();
  renderer->GetActiveCamera()->Azimuth(-20);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
