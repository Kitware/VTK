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

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkParallelRenderManager.h"
#include "vtkPNGWriter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

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
    if (this->Target && this->Target->GetAutomaticEventHandling())
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
vtkCxxSetObjectMacro(vtkSynchronizedRenderers, ParallelController,
  vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkSynchronizedRenderers, CaptureDelegate,
  vtkSynchronizedRenderers);
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

  this->CaptureDelegate = NULL;
  this->AutomaticEventHandling = true;
}

//----------------------------------------------------------------------------
vtkSynchronizedRenderers::~vtkSynchronizedRenderers()
{
  this->SetCaptureDelegate(0);

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
    if (this->CaptureDelegate &&
      this->CaptureDelegate->GetAutomaticEventHandling() == false)
      {
      this->CaptureDelegate->HandleStartRender();
      }
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

  if (this->CaptureDelegate &&
    this->CaptureDelegate->GetAutomaticEventHandling() == false)
    {
    this->CaptureDelegate->HandleStartRender();
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
  if (this->CaptureDelegate &&
    this->CaptureDelegate->GetAutomaticEventHandling() == false)
    {
    this->CaptureDelegate->HandleEndRender();
    }

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
    if (this->ImageReductionFactor > 1 && this->ParallelRendering)
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
vtkSynchronizedRenderers::vtkRawImage&
vtkSynchronizedRenderers::CaptureRenderedImage()
{
  vtkRawImage& rawImage =
    (this->ImageReductionFactor == 1)?
    this->FullImage : this->ReducedImage;

  if (!rawImage.IsValid())
    {
    if (this->CaptureDelegate)
      {
      rawImage = this->CaptureDelegate->CaptureRenderedImage();
      }
    else
      {
      rawImage.Capture(this->Renderer);
      }
    }

  return rawImage;
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

////----------------------------------------------------------------------------
//void vtkSynchronizedRenderers::ResetCamera()
//{
//  if (!this->ParallelController)
//    {
//    vtkErrorMacro("No controller set.");
//    return;
//    }
//
//  if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
//    {
//    // TODO: gather information about bounds from every one and then reset the
//    // camera on the root node alone. Other processes will get the updated
//    // camera position when render gets called.
//    }
//}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::CollectiveExpandForVisiblePropBounds(
  double bounds[6])
{
  // get local bounds.
  double local_bounds[6];
  this->Renderer->ComputeVisiblePropBounds(local_bounds);

  // merge local bounds into the bounds passed in to this function call.
  vtkBoundingBox box;
  box.AddBounds(local_bounds);
  box.AddBounds(bounds);
  box.GetBounds(bounds);

  if (this->ParallelController->IsA("vtkMPIController"))
    {
    double min_bounds[3] = {bounds[0], bounds[2], bounds[4]};
    double max_bounds[3] = {bounds[1], bounds[3], bounds[5]};
    double min_result[3], max_result[3];
    this->ParallelController->AllReduce(min_bounds, min_result, 3,
      vtkCommunicator::MIN_OP);
    this->ParallelController->AllReduce(max_bounds, max_result, 3,
      vtkCommunicator::MAX_OP);
    bounds[0] = min_result[0];
    bounds[2] = min_result[1];
    bounds[4] = min_result[2];
    bounds[1] = max_result[0];
    bounds[3] = max_result[1];
    bounds[5] = max_result[2];
    }
  else
    {
    // since vtkSocketController does not support such reduction operation, we
    // simply use point-to-point communication.
    double other_bounds[6];
    if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
      {
      this->ParallelController->Send(bounds, 6, 1, COMPUTE_BOUNDS_TAG);
      this->ParallelController->Receive(other_bounds, 6, 1, COMPUTE_BOUNDS_TAG);
      }
    else
      {
      this->ParallelController->Receive(other_bounds, 6, 1, COMPUTE_BOUNDS_TAG);
      this->ParallelController->Send(bounds, 6, 1, COMPUTE_BOUNDS_TAG);
      }

    vtkBoundingBox bbox;
    bbox.AddBounds(bounds);
    bbox.AddBounds(other_bounds);
    bbox.GetBounds(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageReductionFactor: "
    << this->ImageReductionFactor << endl;
  os << indent << "WriteBackImages: " << this->WriteBackImages << endl;
  os << indent << "RootProcessId: " << this->RootProcessId << endl;
  os << indent << "ParallelRendering: " << this->ParallelRendering << endl;
  os << indent << "AutomaticEventHandling: "
    << this->AutomaticEventHandling << endl;
  os << indent << "CaptureDelegate: ";
  if(this->CaptureDelegate==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->CaptureDelegate << endl;
    }

  os << indent << "Renderer: ";
  if(this->Renderer==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->Renderer << endl;
    }

  os << indent << "ParallelController: ";
  if(this->ParallelController==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->ParallelController << endl;
    }

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
void vtkSynchronizedRenderers::vtkRawImage::Initialize(
  int dx, int dy, vtkUnsignedCharArray* data)
{
  this->Data = data;
  this->Size[0] = dx;
  this->Size[1] = dy;
}

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
void vtkSynchronizedRenderers::vtkRawImage::SaveAsPNG(const char* filename)
{
  if (!this->IsValid())
    {
    vtkGenericWarningMacro("Image is not valid. Cannot save PNG.");
    return;
    }

  vtkImageData* img = vtkImageData::New();
  img->SetScalarTypeToUnsignedChar();
  img->SetNumberOfScalarComponents(
    this->Data->GetNumberOfComponents());
  img->SetDimensions(this->Size[0], this->Size[1], 1);
  img->AllocateScalars();
  memcpy(img->GetScalarPointer(),
    this->GetRawPtr()->GetVoidPointer(0),
    sizeof(unsigned char)*this->Size[0]*this->Size[1]*
    this->Data->GetNumberOfComponents());

  vtkPNGWriter* writer = vtkPNGWriter::New();
  writer->SetFileName(filename);
  writer->SetInput(img);
  writer->Write();
  writer->Delete();
  img->Delete();
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
  double viewport[4];
  ren->GetViewport(viewport);
  const int* window_size = ren->GetVTKWindow()->GetActualSize();

  glEnable(GL_SCISSOR_TEST);
  glViewport(
    static_cast<GLint>(viewport[0]*window_size[0]),
    static_cast<GLint>(viewport[1]*window_size[1]),
    static_cast<GLsizei>((viewport[2]-viewport[0])*window_size[0]),
    static_cast<GLsizei>((viewport[3]-viewport[1])*window_size[1]));
  glScissor(
    static_cast<GLint>(viewport[0]*window_size[0]),
    static_cast<GLint>(viewport[1]*window_size[1]),
    static_cast<GLsizei>((viewport[2]-viewport[0])*window_size[0]),
    static_cast<GLsizei>((viewport[3]-viewport[1])*window_size[1]));
  ren->Clear();
  return this->PushToFrameBuffer();
}

//----------------------------------------------------------------------------
bool vtkSynchronizedRenderers::vtkRawImage::PushToFrameBuffer()
{
  if (!this->IsValid())
    {
    vtkGenericWarningMacro("Image not valid. Cannot push to screen.");
    return false;
    }

  glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT| GL_TEXTURE_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);

  GLuint tex=0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  if (this->Data->GetNumberOfComponents()==4)
    {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
      this->GetWidth(), this->GetHeight(), 0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      static_cast<const GLvoid*>(
        this->GetRawPtr()->GetVoidPointer(0)));
    }
  else if (this->Data->GetNumberOfComponents()==3)
    {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
      this->GetWidth(), this->GetHeight(), 0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      static_cast<const GLvoid*>(
        this->GetRawPtr()->GetVoidPointer(0)));
    }
  else
    {
    vtkGenericWarningMacro("Only 3 or 4 component images are handled.");
    }
  glBindTexture(GL_TEXTURE_2D, tex);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_DEPTH_TEST);
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
  window_size[0] = ren->GetVTKWindow()->GetActualSize()[0];
  window_size[1] = ren->GetVTKWindow()->GetActualSize()[1];

  int image_size[2];
  image_size[0] = static_cast<int>(window_size[0] * (viewport[2]-viewport[0]));
  image_size[1] = static_cast<int>(window_size[1] * (viewport[3]-viewport[1]));

  // using RGBA always?
  this->Resize(image_size[0], image_size[1], 4);

  ren->GetRenderWindow()->GetRGBACharPixelData(
    static_cast<int>(window_size[0] * viewport[0]),
    static_cast<int>(window_size[1] * viewport[1]),
    static_cast<int>(window_size[0] * viewport[2])-1,
    static_cast<int>(window_size[1] * viewport[3])-1,
    ren->GetRenderWindow()->GetDoubleBuffer()? 0 : 1,
    this->GetRawPtr()); 
  this->MarkValid();
  return true;
}
