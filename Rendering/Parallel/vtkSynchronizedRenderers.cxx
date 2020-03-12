/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedRenderers.cxx

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
#include "vtkFXAAOptions.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFXAAFilter.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkPNGWriter.h"
#include "vtkParallelRenderManager.h"
#include "vtkRenderWindow.h"

#include <cassert>

//----------------------------------------------------------------------------
class vtkSynchronizedRenderers::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
  {
    vtkObserver* obs = new vtkObserver();
    obs->Target = nullptr;
    return obs;
  }

  void Execute(vtkObject*, unsigned long eventId, void*) override
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
vtkCxxSetObjectMacro(vtkSynchronizedRenderers, ParallelController, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkSynchronizedRenderers, CaptureDelegate, vtkSynchronizedRenderers);
//----------------------------------------------------------------------------
vtkSynchronizedRenderers::vtkSynchronizedRenderers()
  : LastBackground{ 0, 0, 0 }
  , LastBackgroundAlpha(0)
  , LastTexturedBackground(false)
  , LastGradientBackground(false)
  , FixBackground(false)
{
  this->Observer = vtkSynchronizedRenderers::vtkObserver::New();
  this->Observer->Target = this;

  this->UseFXAA = false;
  this->FXAAFilter = nullptr;

  this->Renderer = nullptr;
  this->ParallelController = nullptr;
  this->ParallelRendering = true;
  this->ImageReductionFactor = 1;

  this->WriteBackImages = true;
  this->RootProcessId = 0;

  this->CaptureDelegate = nullptr;
  this->AutomaticEventHandling = true;
}

//----------------------------------------------------------------------------
vtkSynchronizedRenderers::~vtkSynchronizedRenderers()
{
  this->SetCaptureDelegate(nullptr);

  this->Observer->Target = nullptr;

  this->SetRenderer(nullptr);
  this->SetParallelController(nullptr);
  this->Observer->Delete();
  this->Observer = nullptr;

  // vtkOpenGLFXAAFilter is only available on opengl2:
  if (this->FXAAFilter)
  {
    this->FXAAFilter->Delete();
    this->FXAAFilter = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::SetRenderer(vtkRenderer* renderer)
{
  if (this->Renderer != renderer)
  {
    if (this->Renderer)
    {
      this->Renderer->RemoveObserver(this->Observer);
    }

    // The renderer should be OpenGL ...
    vtkOpenGLRenderer* glRenderer = vtkOpenGLRenderer::SafeDownCast(renderer);

    if (renderer && !glRenderer)
    {
      vtkErrorMacro("Received non OpenGL renderer");
      assert(false);
    }

    vtkSetObjectBodyMacro(Renderer, vtkOpenGLRenderer, glRenderer);
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
  if (!this->Renderer || !this->ParallelRendering || !this->ParallelController)
  {
    if (this->CaptureDelegate && this->CaptureDelegate->GetAutomaticEventHandling() == false)
    {
      this->CaptureDelegate->HandleStartRender();
    }
    return;
  }

  this->Image.MarkInValid();

  // disable FXAA when parallel rendering. We'll do the FXAA pass after the
  // compositing stage. This avoid any seam artifacts from creeping in.
  this->UseFXAA = this->Renderer->GetUseFXAA();
  this->Renderer->SetUseFXAA(false);

  if (this->FixBackground)
  {
    this->Renderer->GetBackground(this->LastBackground);
    this->LastBackgroundAlpha = this->Renderer->GetBackgroundAlpha();
    this->LastTexturedBackground = this->Renderer->GetTexturedBackground();
    this->LastGradientBackground = this->Renderer->GetGradientBackground();

    this->Renderer->SetBackground(0, 0, 0);
    this->Renderer->SetBackgroundAlpha(0);
    this->Renderer->SetTexturedBackground(false);
    this->Renderer->SetGradientBackground(false);
  }

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
    this->Renderer->SetViewport(this->LastViewport[0] / this->ImageReductionFactor,
      this->LastViewport[1] / this->ImageReductionFactor,
      this->LastViewport[2] / this->ImageReductionFactor,
      this->LastViewport[3] / this->ImageReductionFactor);
  }

  if (this->CaptureDelegate && this->CaptureDelegate->GetAutomaticEventHandling() == false)
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
  if (this->CaptureDelegate && this->CaptureDelegate->GetAutomaticEventHandling() == false)
  {
    this->CaptureDelegate->HandleEndRender();
  }

  if (!this->Renderer || !this->ParallelRendering || !this->ParallelController)
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
    if (this->GetImageReductionFactor() > 1 || this->FixBackground)
    {
      this->CaptureRenderedImage();
    }
  }

  if (this->FixBackground)
  {
    // restore background values.
    this->Renderer->SetBackground(this->LastBackground);
    this->Renderer->SetBackgroundAlpha(this->LastBackgroundAlpha);
    this->Renderer->SetTexturedBackground(this->LastTexturedBackground);
    this->Renderer->SetGradientBackground(this->LastGradientBackground);
  }

  // restore viewport before `PushImageToScreen`, but after
  // `CaptureRenderedImage`.
  this->Renderer->SetViewport(this->LastViewport);

  if (this->WriteBackImages)
  {
    this->PushImageToScreen();
  }

  // restore FXAA state.
  this->Renderer->SetUseFXAA(this->UseFXAA);
  this->UseFXAA = false;
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::MasterEndRender() {}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::SlaveEndRender() {}

//----------------------------------------------------------------------------
vtkSynchronizedRenderers::vtkRawImage& vtkSynchronizedRenderers::CaptureRenderedImage()
{
  vtkRawImage& rawImage = this->Image;
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
  vtkRawImage& rawImage = this->Image;
  if (!rawImage.IsValid())
  {
    return;
  }

  rawImage.PushToViewport(this->Renderer);

  vtkHardwareSelector* sel = this->Renderer->GetSelector();
  if (sel)
  {
    sel->SavePixelBuffer(sel->GetCurrentPass());
  }

  if (this->UseFXAA)
  {
    if (!this->FXAAFilter)
    {
      this->FXAAFilter = vtkOpenGLFXAAFilter::New();
    }
    this->FXAAFilter->UpdateConfiguration(this->Renderer->GetFXAAOptions());
    this->FXAAFilter->Execute(this->Renderer);
  }
}

////----------------------------------------------------------------------------
// void vtkSynchronizedRenderers::ResetCamera()
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
void vtkSynchronizedRenderers::CollectiveExpandForVisiblePropBounds(double bounds[6])
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
    double min_bounds[3] = { bounds[0], bounds[2], bounds[4] };
    double max_bounds[3] = { bounds[1], bounds[3], bounds[5] };
    double min_result[3], max_result[3];
    this->ParallelController->AllReduce(min_bounds, min_result, 3, vtkCommunicator::MIN_OP);
    this->ParallelController->AllReduce(max_bounds, max_result, 3, vtkCommunicator::MAX_OP);
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

  os << indent << "ImageReductionFactor: " << this->ImageReductionFactor << endl;
  os << indent << "WriteBackImages: " << this->WriteBackImages << endl;
  os << indent << "FixBackground: " << this->FixBackground << endl;
  os << indent << "RootProcessId: " << this->RootProcessId << endl;
  os << indent << "ParallelRendering: " << this->ParallelRendering << endl;
  os << indent << "AutomaticEventHandling: " << this->AutomaticEventHandling << endl;
  os << indent << "CaptureDelegate: ";
  if (this->CaptureDelegate == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->CaptureDelegate << endl;
  }

  os << indent << "Renderer: ";
  if (this->Renderer == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->Renderer << endl;
  }

  os << indent << "ParallelController: ";
  if (this->ParallelController == nullptr)
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
  stream << 1023 << this->ImageReductionFactor << this->Draw << this->CameraParallelProjection
         << this->Viewport[0] << this->Viewport[1] << this->Viewport[2] << this->Viewport[3]
         << this->CameraPosition[0] << this->CameraPosition[1] << this->CameraPosition[2]
         << this->CameraFocalPoint[0] << this->CameraFocalPoint[1] << this->CameraFocalPoint[2]
         << this->CameraViewUp[0] << this->CameraViewUp[1] << this->CameraViewUp[2]
         << this->CameraWindowCenter[0] << this->CameraWindowCenter[1]
         << this->CameraClippingRange[0] << this->CameraClippingRange[1] << this->CameraViewAngle
         << this->CameraParallelScale << this->EyeTransformMatrix[0] << this->EyeTransformMatrix[1]
         << this->EyeTransformMatrix[2] << this->EyeTransformMatrix[3]
         << this->EyeTransformMatrix[4] << this->EyeTransformMatrix[5]
         << this->EyeTransformMatrix[6] << this->EyeTransformMatrix[7]
         << this->EyeTransformMatrix[8] << this->EyeTransformMatrix[9]
         << this->EyeTransformMatrix[10] << this->EyeTransformMatrix[11]
         << this->EyeTransformMatrix[12] << this->EyeTransformMatrix[13]
         << this->EyeTransformMatrix[14] << this->EyeTransformMatrix[15]
         << this->ModelTransformMatrix[0] << this->ModelTransformMatrix[1]
         << this->ModelTransformMatrix[2] << this->ModelTransformMatrix[3]
         << this->ModelTransformMatrix[4] << this->ModelTransformMatrix[5]
         << this->ModelTransformMatrix[6] << this->ModelTransformMatrix[7]
         << this->ModelTransformMatrix[8] << this->ModelTransformMatrix[9]
         << this->ModelTransformMatrix[10] << this->ModelTransformMatrix[11]
         << this->ModelTransformMatrix[12] << this->ModelTransformMatrix[13]
         << this->ModelTransformMatrix[14] << this->ModelTransformMatrix[15];
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
  stream >> this->ImageReductionFactor >> this->Draw >> this->CameraParallelProjection >>
    this->Viewport[0] >> this->Viewport[1] >> this->Viewport[2] >> this->Viewport[3] >>
    this->CameraPosition[0] >> this->CameraPosition[1] >> this->CameraPosition[2] >>
    this->CameraFocalPoint[0] >> this->CameraFocalPoint[1] >> this->CameraFocalPoint[2] >>
    this->CameraViewUp[0] >> this->CameraViewUp[1] >> this->CameraViewUp[2] >>
    this->CameraWindowCenter[0] >> this->CameraWindowCenter[1] >> this->CameraClippingRange[0] >>
    this->CameraClippingRange[1] >> this->CameraViewAngle >> this->CameraParallelScale >>
    this->EyeTransformMatrix[0] >> this->EyeTransformMatrix[1] >> this->EyeTransformMatrix[2] >>
    this->EyeTransformMatrix[3] >> this->EyeTransformMatrix[4] >> this->EyeTransformMatrix[5] >>
    this->EyeTransformMatrix[6] >> this->EyeTransformMatrix[7] >> this->EyeTransformMatrix[8] >>
    this->EyeTransformMatrix[9] >> this->EyeTransformMatrix[10] >> this->EyeTransformMatrix[11] >>
    this->EyeTransformMatrix[12] >> this->EyeTransformMatrix[13] >> this->EyeTransformMatrix[14] >>
    this->EyeTransformMatrix[15] >> this->ModelTransformMatrix[0] >>
    this->ModelTransformMatrix[1] >> this->ModelTransformMatrix[2] >>
    this->ModelTransformMatrix[3] >> this->ModelTransformMatrix[4] >>
    this->ModelTransformMatrix[5] >> this->ModelTransformMatrix[6] >>
    this->ModelTransformMatrix[7] >> this->ModelTransformMatrix[8] >>
    this->ModelTransformMatrix[9] >> this->ModelTransformMatrix[10] >>
    this->ModelTransformMatrix[11] >> this->ModelTransformMatrix[12] >>
    this->ModelTransformMatrix[13] >> this->ModelTransformMatrix[14] >>
    this->ModelTransformMatrix[15];
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

  vtkMatrix4x4* eyeTransformationMatrix = cam->GetEyeTransformMatrix();
  vtkMatrix4x4* modelTransformationMatrix = cam->GetModelTransformMatrix();
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      this->EyeTransformMatrix[i * 4 + j] = eyeTransformationMatrix->GetElement(i, j);
      this->ModelTransformMatrix[i * 4 + j] = modelTransformationMatrix->GetElement(i, j);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::RendererInfo::CopyTo(vtkRenderer* ren)
{
  vtkCamera* cam = ren->GetActiveCamera();
  ren->SetDraw(this->Draw);
  cam->SetParallelProjection(this->CameraParallelProjection);
  // ren->SetViewport(this->Viewport);
  cam->SetPosition(this->CameraPosition);
  cam->SetFocalPoint(this->CameraFocalPoint);
  cam->SetViewUp(this->CameraViewUp);
  cam->SetWindowCenter(this->CameraWindowCenter[0], this->CameraWindowCenter[1]);
  cam->SetClippingRange(this->CameraClippingRange);
  cam->SetViewAngle(this->CameraViewAngle);
  cam->SetParallelScale(this->CameraParallelScale);

  // We reuse vtkMatrix4x4 objects present on the camera and then use
  // vtkMatrix4x4::SetElement(). This avoids modifying the mtime of the
  // camera unless anything truly changed.
  vtkMatrix4x4* eyeTransformationMatrix = cam->GetEyeTransformMatrix();
  vtkMatrix4x4* modelTransformationMatrix = cam->GetModelTransformMatrix();
  assert(eyeTransformationMatrix && modelTransformationMatrix);
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      eyeTransformationMatrix->SetElement(i, j, this->EyeTransformMatrix[i * 4 + j]);
      modelTransformationMatrix->SetElement(i, j, this->ModelTransformMatrix[i * 4 + j]);
    }
  }
}

//****************************************************************************
// vtkSynchronizedRenderers::vtkRawImage Methods
//****************************************************************************

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::vtkRawImage::Initialize(int dx, int dy, vtkUnsignedCharArray* data)
{
  this->Data = data;
  this->Size[0] = dx;
  this->Size[1] = dy;
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderers::vtkRawImage::Allocate(int dx, int dy, int numcomps)
{
  if (dx * dy <= this->Data->GetNumberOfTuples() && this->Data->GetNumberOfComponents() == numcomps)
  {
    this->Size[0] = dx;
    this->Size[1] = dy;
    return;
  }

  this->Data = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Data->SetNumberOfComponents(numcomps);
  this->Data->SetNumberOfTuples(dx * dy);
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
  img->SetDimensions(this->Size[0], this->Size[1], 1);
  img->AllocateScalars(VTK_UNSIGNED_CHAR, this->Data->GetNumberOfComponents());
  memcpy(img->GetScalarPointer(), this->GetRawPtr()->GetVoidPointer(0),
    sizeof(unsigned char) * this->Size[0] * this->Size[1] * this->Data->GetNumberOfComponents());

  vtkPNGWriter* writer = vtkPNGWriter::New();
  writer->SetFileName(filename);
  writer->SetInputData(img);
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

  int size[2], low_point[2];
  ren->GetTiledSizeAndOrigin(&size[0], &size[1], &low_point[0], &low_point[1]);
  vtkLogF(TRACE, "GetTiledSizeAndOrigin(w=%d, h=%d, x=%d, y=%d)", size[0], size[1], low_point[0],
    low_point[1]);
  if (size[0] <= 0 || size[1] <= 0)
  {
    vtkGenericWarningMacro("Viewport empty. Cannot push to screen.");
    return false;
  }

  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow())->GetState();
  ostate->vtkglEnable(GL_SCISSOR_TEST);
  ostate->vtkglViewport(low_point[0], low_point[1], size[0], size[1]);
  ostate->vtkglScissor(low_point[0], low_point[1], size[0], size[1]);

  ren->Clear();
  return this->PushToFrameBuffer(ren);
}

//----------------------------------------------------------------------------
bool vtkSynchronizedRenderers::vtkRawImage::PushToFrameBuffer(vtkRenderer* ren)
{
  if (!this->IsValid())
  {
    vtkGenericWarningMacro("Image not valid. Cannot push to screen.");
    return false;
  }

  vtkOpenGLClearErrorMacro();
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkRawImage::PushToViewport begin");
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  // framebuffers have their color premultiplied by alpha.
  vtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);
  ostate->vtkglEnable(GL_BLEND);
  ostate->vtkglBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  int size[2], low_point[2];
  ren->GetTiledSizeAndOrigin(&size[0], &size[1], &low_point[0], &low_point[1]);

  renWin->DrawPixels(low_point[0], low_point[1], low_point[0] + size[0] - 1,
    low_point[1] + size[1] - 1, 0, 0, this->GetWidth() - 1, this->GetHeight() - 1, this->GetWidth(),
    this->GetHeight(), this->Data->GetNumberOfComponents(), VTK_UNSIGNED_CHAR,
    this->GetRawPtr()->GetVoidPointer(0));

  vtkOpenGLStaticCheckErrorMacro("failed after PushToFrameBuffer");
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkRawImage::PushToViewport end");
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

  int viewport_in_pixels[4];
  viewport_in_pixels[0] = static_cast<int>(window_size[0] * viewport[0]);
  viewport_in_pixels[1] = static_cast<int>(window_size[1] * viewport[1]);
  viewport_in_pixels[2] = static_cast<int>(window_size[0] * viewport[2]) - 1;
  viewport_in_pixels[3] = static_cast<int>(window_size[1] * viewport[3]) - 1;

  // we need to ensure that the size computation is always done in pixels,
  // otherwise we end up with rounding issues. In short, avoid doing
  // additions/subtractions using normalized viewport coordinates. Those are
  // better done in pixels.
  int image_size[2];
  image_size[0] = viewport_in_pixels[2] - viewport_in_pixels[0] + 1;
  image_size[1] = viewport_in_pixels[3] - viewport_in_pixels[1] + 1;

  // using RGBA always?
  this->Resize(image_size[0], image_size[1], 4);

  ren->GetRenderWindow()->GetRGBACharPixelData(viewport_in_pixels[0], viewport_in_pixels[1],
    viewport_in_pixels[2], viewport_in_pixels[3], ren->GetRenderWindow()->GetDoubleBuffer() ? 0 : 1,
    this->GetRawPtr(),
    /*right=*/ren->GetActiveCamera()->GetLeftEye() == 0);

  // if selecting then pass the processed pixel buffer
  vtkHardwareSelector* sel = ren->GetSelector();
  if (sel)
  {
    unsigned char* passdata = sel->GetPixelBuffer(sel->GetCurrentPass());
    unsigned char* destdata = static_cast<unsigned char*>(this->GetRawPtr()->GetVoidPointer(0));
    if (passdata && destdata)
    {
      unsigned int* area = sel->GetArea();
      unsigned int passwidth = area[2] - area[0] + 1;
      for (int y = 0; y < image_size[1]; ++y)
      {
        for (int x = 0; x < image_size[0]; ++x)
        {
          unsigned char* pdptr = passdata + (y * passwidth + x) * 3;
          destdata[0] = pdptr[0];
          destdata[1] = pdptr[1];
          destdata[2] = pdptr[2];
          destdata += 4;
        }
      }
    }
  }

  this->MarkValid();
  return true;
}

vtkRenderer* vtkSynchronizedRenderers::GetRenderer()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Render of "
                << this->Renderer);
  return this->Renderer;
}
