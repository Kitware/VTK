/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSynchronizedRenderers.h"

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkParallelRenderManager.h"

#include "vtkgl.h"


//----------------------------------------------------------------------------
class vtkSynchronizedRenderers::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
    {
    vtkObserver* obs = new vtkObserver();
    obs->Target = NULL;
    return obs;
    }

  virtual void Execute(vtkObject *, unsigned long eventId, void *)
    {
    if (this->Target)
      {
      switch (eventId)
        {
      case vtkCommand::StartEvent:
        this->Target->HandleStartRender();
        break;

      case vtkCommand::EndEvent:
        this->Target->HandleEndRender();
        break;

      case vtkCommand::AbortCheckEvent:
        this->Target->HandleAbortRender();
        break;
        }
      }
    }

  vtkSynchronizedRenderers* Target;
};

vtkStandardNewMacro(vtkSynchronizedRenderers);
vtkCxxRevisionMacro(vtkSynchronizedRenderers, "$Revision$");
vtkCxxSetObjectMacro(vtkSynchronizedRenderers, ParallelController,
  vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkSynchronizedRenderers::vtkSynchronizedRenderers()
{
  this->Observer = vtkSynchronizedRenderers::vtkObserver::New();
  this->Observer->Target = this;

  this->Renderer = 0;
  this->ParallelController = 0;
  this->ParallelRendering = true;
  this->ImageReductionFactor = 1;

  this->WriteBackImages = true;
  this->RootProcessId = 0;
}

//----------------------------------------------------------------------------
vtkSynchronizedRenderers::~vtkSynchronizedRenderers()
{
  this->Observer->Target = 0;

  this->SetRenderer(0);
  this->SetParallelController(0);
  this->Observer->Delete();
  this->Observer = 0;
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::SetRenderer(vtkRenderer* renderer)
{
  if (this->Renderer  != renderer)
    {
    if (this->Renderer)
      {
      this->Renderer->RemoveObserver(this->Observer);
      }
    vtkSetObjectBodyMacro(Renderer, vtkRenderer, renderer);
    if (this->Renderer)
      {
      this->Renderer->AddObserver(vtkCommand::StartEvent, this->Observer);
      this->Renderer->AddObserver(vtkCommand::EndEvent, this->Observer);
      // this->Renderer->AddObserver(vtkCommand::AbortCheckEvent, this->Observer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::HandleStartRender()
{
  if (!this->Renderer || !this->ParallelRendering ||
    !this->ParallelController)
    {
    return;
    }

  this->ReducedImage.MarkInValid();
  this->FullImage.MarkInValid();

  if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
    {
    this->MasterStartRender();
    }
  else
    {
    this->SlaveStartRender();
    }

  this->Renderer->GetViewport(this->LastViewport);
  if (this->ImageReductionFactor > 1)
    {
    this->Renderer->SetViewport(
      this->LastViewport[0]/this->ImageReductionFactor,
      this->LastViewport[1]/this->ImageReductionFactor,
      this->LastViewport[2]/this->ImageReductionFactor,
      this->LastViewport[3]/this->ImageReductionFactor);
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::MasterStartRender()
{
  RendererInfo renInfo;
  renInfo.ImageReductionFactor = this->GetImageReductionFactor();
  renInfo.CopyFrom(this->Renderer);
  vtkMultiProcessStream stream;
  renInfo.Save(stream);

  this->ParallelController->Broadcast(stream, this->RootProcessId);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::SlaveStartRender()
{
  vtkMultiProcessStream stream;
  this->ParallelController->Broadcast(stream, this->RootProcessId);

  RendererInfo renInfo;
  renInfo.Restore(stream);
  renInfo.CopyTo(this->Renderer);
  this->SetImageReductionFactor(renInfo.ImageReductionFactor);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::HandleEndRender()
{
  if (!this->Renderer || !this->ParallelRendering ||
    !this->ParallelController)
    {
    return;
    }

  if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
    {
    this->MasterEndRender();
    }
  else
    {
    this->SlaveEndRender();
    }


  if (this->WriteBackImages)
    {
    if (this->ImageReductionFactor && this->ParallelRendering)
      {
      this->CaptureRenderedImage();
      }

    this->PushImageToScreen();
    }

  this->Renderer->SetViewport(this->LastViewport);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::MasterEndRender()
{
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::SlaveEndRender()
{
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::CaptureRenderedImage()
{
  vtkRawImage& rawImage =
    (this->ImageReductionFactor == 1)?
    this->FullImage : this->ReducedImage;

  if (rawImage.IsValid())
    {
    return;
    }

  rawImage.Capture(this->Renderer);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::PushImageToScreen()
{
  vtkRawImage& rawImage =
    (this->ImageReductionFactor == 1)?
    this->FullImage : this->ReducedImage;

  if (!rawImage.IsValid())
    {
    return;
    }

  rawImage.PushToViewport(this->Renderer);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::ResetCamera()
{
  if (!this->ParallelController)
    {
    vtkErrorMacro("No controller set.");
    return;
    }

  if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
    {
    // TODO: gather information about bounds from every one and then reset the
    // camera on the root node alone. Other processes will get the updated
    // camera position when render gets called.
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// ** INFO OBJECT METHODS ***
//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::RendererInfo::Save(vtkMultiProcessStream& stream)
{
  stream << 1023
         << this->ImageReductionFactor
         << this->Draw
         << this->CameraParallelProjection
         << this->Viewport[0] << this->Viewport[1]
         << this->Viewport[2] << this->Viewport[3]
         << this->CameraPosition[0]
         << this->CameraPosition[1]
         << this->CameraPosition[2]
         << this->CameraFocalPoint[0]
         << this->CameraFocalPoint[1]
         << this->CameraFocalPoint[2]
         << this->CameraViewUp[0]
         << this->CameraViewUp[1]
         << this->CameraViewUp[2]
         << this->CameraWindowCenter[0]
         << this->CameraWindowCenter[1]
         << this->CameraClippingRange[0]
         << this->CameraClippingRange[1]
         << this->CameraViewAngle
         << this->CameraParallelScale;
}

//----------------------------------------------------------------------------
bool vtkSynchronizedRenderers::RendererInfo::Restore(vtkMultiProcessStream& stream)
{
  int tag;
  stream >> tag;
  if (tag != 1023)
    {
    return false;
    }
  stream >> this->ImageReductionFactor
         >> this->Draw
         >> this->CameraParallelProjection
         >> this->Viewport[0] 
         >> this->Viewport[1]
         >> this->Viewport[2] 
         >> this->Viewport[3]
         >> this->CameraPosition[0]
         >> this->CameraPosition[1]
         >> this->CameraPosition[2]
         >> this->CameraFocalPoint[0]
         >> this->CameraFocalPoint[1]
         >> this->CameraFocalPoint[2]
         >> this->CameraViewUp[0]
         >> this->CameraViewUp[1]
         >> this->CameraViewUp[2]
         >> this->CameraWindowCenter[0]
         >> this->CameraWindowCenter[1]
         >> this->CameraClippingRange[0]
         >> this->CameraClippingRange[1]
         >> this->CameraViewAngle
         >> this->CameraParallelScale;
  return true;
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::RendererInfo::CopyFrom(vtkRenderer* ren)
{
  vtkCamera* cam = ren->GetActiveCamera();
  this->Draw = ren->GetDraw();
  this->CameraParallelProjection = cam->GetParallelProjection();
  ren->GetViewport(this->Viewport);
  cam->GetPosition(this->CameraPosition);
  cam->GetFocalPoint(this->CameraFocalPoint);
  cam->GetViewUp(this->CameraViewUp);
  cam->GetWindowCenter(this->CameraWindowCenter);
  cam->GetClippingRange(this->CameraClippingRange);
  this->CameraViewAngle = cam->GetViewAngle();
  this->CameraParallelScale = cam->GetParallelScale();
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::RendererInfo::CopyTo(vtkRenderer* ren)
{
  vtkCamera* cam = ren->GetActiveCamera();
  ren->SetDraw(this->Draw);
  cam->SetParallelProjection(this->CameraParallelProjection);
  //ren->SetViewport(this->Viewport);
  cam->SetPosition(this->CameraPosition);
  cam->SetFocalPoint(this->CameraFocalPoint);
  cam->SetViewUp(this->CameraViewUp);
  cam->SetWindowCenter(this->CameraWindowCenter[0],
    this->CameraWindowCenter[1]);
  cam->SetClippingRange(this->CameraClippingRange);
  cam->SetViewAngle(this->CameraViewAngle);
  cam->SetParallelScale(this->CameraParallelScale);
}


//****************************************************************************
// vtkSynchronizedRenderers::vtkRawImage Methods
//****************************************************************************

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::vtkRawImage::Allocate(int dx, int dy, int numcomps)
{
  if (dx*dy < this->Data->GetNumberOfTuples() &&
    this->Data->GetNumberOfComponents() == numcomps)
    {
    this->Size[0] = dx;
    this->Size[1] = dy;
    return;
    }

  this->Data = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Data->SetNumberOfComponents(numcomps);
  this->Data->SetNumberOfTuples(dx*dy);
  this->Size[0] = dx;
  this->Size[1] = dy;
}

//----------------------------------------------------------------------------
bool vtkSynchronizedRenderers::vtkRawImage::PushToViewport(vtkRenderer* ren)
{
  if (!this->IsValid())
    {
    vtkGenericWarningMacro("Image not valid. Cannot push to screen.");
    return false;
    }

  // FIXME: This will not work when non-power-of-two textures are not supported.
  double viewport[4] = {0, 0, 1, 1};
  ren->NormalizedDisplayToViewport(viewport[0], viewport[1]);
  ren->NormalizedDisplayToViewport(viewport[2], viewport[3]);

  glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glDisable(GL_SCISSOR_TEST);
  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);

  GLuint tex=0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
    this->GetWidth(), this->GetHeight(), 0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    static_cast<const GLvoid*>(
      this->GetRawPtr()->GetVoidPointer(0)));
  glBindTexture(GL_TEXTURE_2D, tex);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_DEPTH_TEST);
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glEnable(GL_TEXTURE_2D);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(-1.0, -1.0);

  glTexCoord2f(1.0, 0.0);
  glVertex2f(1.0, -1.0);

  glTexCoord2f(1.0, 1.0);
  glVertex2f(1.0, 1.0);

  glTexCoord2f(0.0, 1.0);
  glVertex2f(-1.0, 1.0);
  glEnd();

  glDisable(GL_TEXTURE_2D);
  glDeleteTextures(1, &tex);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
  return true;
}


//----------------------------------------------------------------------------
bool vtkSynchronizedRenderers::vtkRawImage::Capture(vtkRenderer* ren)
{
  double viewport[4];
  ren->GetViewport(viewport);

  int window_size[2];
  window_size[0] = ren->GetVTKWindow()->GetSize()[0];
  window_size[1] = ren->GetVTKWindow()->GetSize()[1];

  int image_size[2];
  image_size[0] = window_size[0] * (viewport[2]-viewport[0]) + 1;
  image_size[1] = window_size[1] * (viewport[3]-viewport[1]) + 1;

  // using RGBA always?
  this->Resize(image_size[0], image_size[1], 4);

  ren->GetRenderWindow()->GetRGBACharPixelData(
    window_size[0] * viewport[0],
    window_size[1] * viewport[1],
    window_size[0] * viewport[2],
    window_size[1] * viewport[3],
    ren->GetRenderWindow()->GetDoubleBuffer()? 0 : 1,
    this->GetRawPtr()); 
  this->MarkValid();
  return true;
}
