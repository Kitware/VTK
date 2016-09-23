/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageViewer2.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageViewer2);

//----------------------------------------------------------------------------
vtkImageViewer2::vtkImageViewer2()
{
  this->RenderWindow    = NULL;
  this->Renderer        = NULL;
  this->ImageActor      = vtkImageActor::New();
  this->WindowLevel     = vtkImageMapToWindowLevelColors::New();
  this->Interactor      = NULL;
  this->InteractorStyle = NULL;

  this->Slice = 0;
  this->FirstRender = 1;
  this->SliceOrientation = vtkImageViewer2::SLICE_ORIENTATION_XY;

  // Setup the pipeline

  vtkRenderWindow *renwin = vtkRenderWindow::New();
  this->SetRenderWindow(renwin);
  renwin->Delete();

  vtkRenderer *ren = vtkRenderer::New();
  this->SetRenderer(ren);
  ren->Delete();

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
vtkImageViewer2::~vtkImageViewer2()
{
  if (this->WindowLevel)
  {
    this->WindowLevel->Delete();
    this->WindowLevel = NULL;
  }

  if (this->ImageActor)
  {
    this->ImageActor->Delete();
    this->ImageActor = NULL;
  }

  if (this->Renderer)
  {
    this->Renderer->Delete();
    this->Renderer = NULL;
  }

  if (this->RenderWindow)
  {
    this->RenderWindow->Delete();
    this->RenderWindow = NULL;
  }

  if (this->Interactor)
  {
    this->Interactor->Delete();
    this->Interactor = NULL;
  }

  if (this->InteractorStyle)
  {
    this->InteractorStyle->Delete();
    this->InteractorStyle = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetupInteractor(vtkRenderWindowInteractor *arg)
{
  if (this->Interactor == arg)
  {
    return;
  }

  this->UnInstallPipeline();

  if (this->Interactor)
  {
    this->Interactor->UnRegister(this);
  }

  this->Interactor = arg;

  if (this->Interactor)
  {
    this->Interactor->Register(this);
  }

  this->InstallPipeline();

  if (this->Renderer)
  {
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetRenderWindow(vtkRenderWindow *arg)
{
  if (this->RenderWindow == arg)
  {
    return;
  }

  this->UnInstallPipeline();

  if (this->RenderWindow)
  {
    this->RenderWindow->UnRegister(this);
  }

  this->RenderWindow = arg;

  if (this->RenderWindow)
  {
    this->RenderWindow->Register(this);
  }

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetRenderer(vtkRenderer *arg)
{
  if (this->Renderer == arg)
  {
    return;
  }

  this->UnInstallPipeline();

  if (this->Renderer)
  {
    this->Renderer->UnRegister(this);
  }

  this->Renderer = arg;

  if (this->Renderer)
  {
    this->Renderer->Register(this);
  }

  this->InstallPipeline();
  this->UpdateOrientation();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetSize(int a,int b)
{
  this->RenderWindow->SetSize(a, b);
}

//----------------------------------------------------------------------------
int* vtkImageViewer2::GetSize()
{
  return this->RenderWindow->GetSize();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::GetSliceRange(int &min, int &max)
{
  vtkAlgorithm *input = this->GetInputAlgorithm();
  if (input)
  {
    input->UpdateInformation();
    int *w_ext = input->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    min = w_ext[this->SliceOrientation * 2];
    max = w_ext[this->SliceOrientation * 2 + 1];
  }
}

//----------------------------------------------------------------------------
int* vtkImageViewer2::GetSliceRange()
{
  vtkAlgorithm *input = this->GetInputAlgorithm();
  if (input)
  {
    input->UpdateInformation();
    return input->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) +
      this->SliceOrientation * 2;
  }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkImageViewer2::GetSliceMin()
{
  int *range = this->GetSliceRange();
  if (range)
  {
    return range[0];
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkImageViewer2::GetSliceMax()
{
  int *range = this->GetSliceRange();
  if (range)
  {
    return range[1];
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetSlice(int slice)
{
  int *range = this->GetSliceRange();
  if (range)
  {
    if (slice < range[0])
    {
      slice = range[0];
    }
    else if (slice > range[1])
    {
      slice = range[1];
    }
  }

  if (this->Slice == slice)
  {
    return;
  }

  this->Slice = slice;
  this->Modified();

  this->UpdateDisplayExtent();
  this->Render();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetSliceOrientation(int orientation)
{
  if (orientation < vtkImageViewer2::SLICE_ORIENTATION_YZ ||
      orientation > vtkImageViewer2::SLICE_ORIENTATION_XY)
  {
    vtkErrorMacro("Error - invalid slice orientation " << orientation);
    return;
  }

  if (this->SliceOrientation == orientation)
  {
    return;
  }

  this->SliceOrientation = orientation;

  // Update the viewer

  int *range = this->GetSliceRange();
  if (range)
  {
    this->Slice = static_cast<int>((range[0] + range[1]) * 0.5);
  }

  this->UpdateOrientation();
  this->UpdateDisplayExtent();

  if (this->Renderer && this->GetInput())
  {
    double scale = this->Renderer->GetActiveCamera()->GetParallelScale();
    this->Renderer->ResetCamera();
    this->Renderer->GetActiveCamera()->SetParallelScale(scale);
  }

  this->Render();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::UpdateOrientation()
{
  // Set the camera position

  vtkCamera *cam = this->Renderer ? this->Renderer->GetActiveCamera() : NULL;
  if (cam)
  {
    switch (this->SliceOrientation)
    {
      case vtkImageViewer2::SLICE_ORIENTATION_XY:
        cam->SetFocalPoint(0,0,0);
        cam->SetPosition(0,0,1); // -1 if medical ?
        cam->SetViewUp(0,1,0);
        break;

      case vtkImageViewer2::SLICE_ORIENTATION_XZ:
        cam->SetFocalPoint(0,0,0);
        cam->SetPosition(0,-1,0); // 1 if medical ?
        cam->SetViewUp(0,0,1);
        break;

      case vtkImageViewer2::SLICE_ORIENTATION_YZ:
        cam->SetFocalPoint(0,0,0);
        cam->SetPosition(1,0,0); // -1 if medical ?
        cam->SetViewUp(0,0,1);
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::UpdateDisplayExtent()
{
  vtkAlgorithm *input = this->GetInputAlgorithm();
  if (!input || !this->ImageActor)
  {
    return;
  }

  input->UpdateInformation();
  vtkInformation* outInfo = input->GetOutputInformation(0);
  int *w_ext = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  // Is the slice in range ? If not, fix it

  int slice_min = w_ext[this->SliceOrientation * 2];
  int slice_max = w_ext[this->SliceOrientation * 2 + 1];
  if (this->Slice < slice_min || this->Slice > slice_max)
  {
    this->Slice = static_cast<int>((slice_min + slice_max) * 0.5);
  }

  // Set the image actor

  switch (this->SliceOrientation)
  {
    case vtkImageViewer2::SLICE_ORIENTATION_XY:
      this->ImageActor->SetDisplayExtent(
        w_ext[0], w_ext[1], w_ext[2], w_ext[3], this->Slice, this->Slice);
      break;

    case vtkImageViewer2::SLICE_ORIENTATION_XZ:
      this->ImageActor->SetDisplayExtent(
        w_ext[0], w_ext[1], this->Slice, this->Slice, w_ext[4], w_ext[5]);
      break;

    case vtkImageViewer2::SLICE_ORIENTATION_YZ:
      this->ImageActor->SetDisplayExtent(
        this->Slice, this->Slice, w_ext[2], w_ext[3], w_ext[4], w_ext[5]);
      break;
  }

  // Figure out the correct clipping range

  if (this->Renderer)
  {
    if (this->InteractorStyle &&
        this->InteractorStyle->GetAutoAdjustCameraClippingRange())
    {
      this->Renderer->ResetCameraClippingRange();
    }
    else
    {
      vtkCamera *cam = this->Renderer->GetActiveCamera();
      if (cam)
      {
        double bounds[6];
        this->ImageActor->GetBounds(bounds);
        double spos = bounds[this->SliceOrientation * 2];
        double cpos = cam->GetPosition()[this->SliceOrientation];
        double range = fabs(spos - cpos);
        double *spacing = outInfo->Get(vtkDataObject::SPACING());
        double avg_spacing =
          (spacing[0] + spacing[1] + spacing[2]) / 3.0;
        cam->SetClippingRange(
          range - avg_spacing * 3.0, range + avg_spacing * 3.0);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetPosition(int a,int b)
{
  this->RenderWindow->SetPosition(a, b);
}

//----------------------------------------------------------------------------
int* vtkImageViewer2::GetPosition()
{
  return this->RenderWindow->GetPosition();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetDisplayId(void *a)
{
  this->RenderWindow->SetDisplayId(a);
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetWindowId(void *a)
{
  this->RenderWindow->SetWindowId(a);
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetParentId(void *a)
{
  this->RenderWindow->SetParentId(a);
}

//----------------------------------------------------------------------------
double vtkImageViewer2::GetColorWindow()
{
  return this->WindowLevel->GetWindow();
}

//----------------------------------------------------------------------------
double vtkImageViewer2::GetColorLevel()
{
  return this->WindowLevel->GetLevel();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetColorWindow(double s)
{
  this->WindowLevel->SetWindow(s);
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetColorLevel(double s)
{
  this->WindowLevel->SetLevel(s);
}

//----------------------------------------------------------------------------
class vtkImageViewer2Callback : public vtkCommand
{
public:
  static vtkImageViewer2Callback *New() { return new vtkImageViewer2Callback; }

  void Execute(vtkObject *caller,
               unsigned long event,
               void *vtkNotUsed(callData)) VTK_OVERRIDE
  {
      if (this->IV->GetInput() == NULL)
      {
        return;
      }

      // Reset

      if (event == vtkCommand::ResetWindowLevelEvent)
      {
        this->IV->GetInputAlgorithm()->UpdateWholeExtent();
        double *range = this->IV->GetInput()->GetScalarRange();
        this->IV->SetColorWindow(range[1] - range[0]);
        this->IV->SetColorLevel(0.5 * (range[1] + range[0]));
        this->IV->Render();
        return;
      }

      // Start

      if (event == vtkCommand::StartWindowLevelEvent)
      {
        this->InitialWindow = this->IV->GetColorWindow();
        this->InitialLevel = this->IV->GetColorLevel();
        return;
      }

      // Adjust the window level here

      vtkInteractorStyleImage *isi =
        static_cast<vtkInteractorStyleImage *>(caller);

      int *size = this->IV->GetRenderWindow()->GetSize();
      double window = this->InitialWindow;
      double level = this->InitialLevel;

      // Compute normalized delta

      double dx = 4.0 *
        (isi->GetWindowLevelCurrentPosition()[0] -
         isi->GetWindowLevelStartPosition()[0]) / size[0];
      double dy = 4.0 *
        (isi->GetWindowLevelStartPosition()[1] -
         isi->GetWindowLevelCurrentPosition()[1]) / size[1];

      // Scale by current values

      if (fabs(window) > 0.01)
      {
        dx = dx * window;
      }
      else
      {
        dx = dx * (window < 0 ? -0.01 : 0.01);
      }
      if (fabs(level) > 0.01)
      {
        dy = dy * level;
      }
      else
      {
        dy = dy * (level < 0 ? -0.01 : 0.01);
      }

      // Abs so that direction does not flip

      if (window < 0.0)
      {
        dx = -1*dx;
      }
      if (level < 0.0)
      {
        dy = -1*dy;
      }

      // Compute new window level

      double newWindow = dx + window;
      double newLevel;
      newLevel = level - dy;

      // Stay away from zero and really

      if (fabs(newWindow) < 0.01)
      {
        newWindow = 0.01*(newWindow < 0 ? -1 : 1);
      }
      if (fabs(newLevel) < 0.01)
      {
        newLevel = 0.01*(newLevel < 0 ? -1 : 1);
      }

      this->IV->SetColorWindow(newWindow);
      this->IV->SetColorLevel(newLevel);
      this->IV->Render();
  }

  vtkImageViewer2 *IV;
  double InitialWindow;
  double InitialLevel;
};

//----------------------------------------------------------------------------
void vtkImageViewer2::InstallPipeline()
{
  if (this->RenderWindow && this->Renderer)
  {
    this->RenderWindow->AddRenderer(this->Renderer);
  }

  if (this->Interactor)
  {
    if (!this->InteractorStyle)
    {
      this->InteractorStyle = vtkInteractorStyleImage::New();
      vtkImageViewer2Callback *cbk = vtkImageViewer2Callback::New();
      cbk->IV = this;
      this->InteractorStyle->AddObserver(
        vtkCommand::WindowLevelEvent, cbk);
      this->InteractorStyle->AddObserver(
        vtkCommand::StartWindowLevelEvent, cbk);
      this->InteractorStyle->AddObserver(
        vtkCommand::ResetWindowLevelEvent, cbk);
      cbk->Delete();
    }

    this->Interactor->SetInteractorStyle(this->InteractorStyle);
    this->Interactor->SetRenderWindow(this->RenderWindow);
  }

  if (this->Renderer && this->ImageActor)
  {
    this->Renderer->AddViewProp(this->ImageActor);
  }

  if (this->ImageActor && this->WindowLevel)
  {
    this->ImageActor->GetMapper()->SetInputConnection(
      this->WindowLevel->GetOutputPort());
  }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::UnInstallPipeline()
{
  if (this->ImageActor)
  {
    this->ImageActor->GetMapper()->SetInputConnection(NULL);
  }

  if (this->Renderer && this->ImageActor)
  {
    this->Renderer->RemoveViewProp(this->ImageActor);
  }

  if (this->RenderWindow && this->Renderer)
  {
    this->RenderWindow->RemoveRenderer(this->Renderer);
  }

  if (this->Interactor)
  {
    this->Interactor->SetInteractorStyle(NULL);
    this->Interactor->SetRenderWindow(NULL);
  }
}

//----------------------------------------------------------------------------
void vtkImageViewer2::Render()
{
  if (this->FirstRender)
  {
    // Initialize the size if not set yet

    vtkAlgorithm *input = this->GetInputAlgorithm();
    if (input)
    {
      input->UpdateInformation();
      int *w_ext = this->GetInputInformation()->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      int xs = 0, ys = 0;

      switch (this->SliceOrientation)
      {
        case vtkImageViewer2::SLICE_ORIENTATION_XY:
        default:
          xs = w_ext[1] - w_ext[0] + 1;
          ys = w_ext[3] - w_ext[2] + 1;
          break;

        case vtkImageViewer2::SLICE_ORIENTATION_XZ:
          xs = w_ext[1] - w_ext[0] + 1;
          ys = w_ext[5] - w_ext[4] + 1;
          break;

        case vtkImageViewer2::SLICE_ORIENTATION_YZ:
          xs = w_ext[3] - w_ext[2] + 1;
          ys = w_ext[5] - w_ext[4] + 1;
          break;
      }

      // if it would be smaller than 150 by 100 then limit to 150 by 100
      if (this->RenderWindow->GetSize()[0] == 0)
      {
        this->RenderWindow->SetSize(
          xs < 150 ? 150 : xs, ys < 100 ? 100 : ys);
      }

      if (this->Renderer)
      {
        this->Renderer->ResetCamera();
        this->Renderer->GetActiveCamera()->SetParallelScale(
          xs < 150 ? 75 : (xs - 1 ) / 2.0);
      }
      this->FirstRender = 0;
    }
  }
  if (this->GetInput())
  {
    this->RenderWindow->Render();
  }
}

//----------------------------------------------------------------------------
const char* vtkImageViewer2::GetWindowName()
{
  return this->RenderWindow->GetWindowName();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetOffScreenRendering(int i)
{
  this->RenderWindow->SetOffScreenRendering(i);
}

//----------------------------------------------------------------------------
int vtkImageViewer2::GetOffScreenRendering()
{
  return this->RenderWindow->GetOffScreenRendering();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetInputData(vtkImageData *in)
{
  this->WindowLevel->SetInputData(in);
  this->UpdateDisplayExtent();
}
//----------------------------------------------------------------------------
vtkImageData* vtkImageViewer2::GetInput()
{
  return vtkImageData::SafeDownCast(this->WindowLevel->GetInput());
}
//----------------------------------------------------------------------------
vtkInformation* vtkImageViewer2::GetInputInformation()
{
  return this->WindowLevel->GetInputInformation();
}
//----------------------------------------------------------------------------
vtkAlgorithm* vtkImageViewer2::GetInputAlgorithm()
{
  return this->WindowLevel->GetInputAlgorithm();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::SetInputConnection(vtkAlgorithmOutput* input)
{
  this->WindowLevel->SetInputConnection(input);
  this->UpdateDisplayExtent();
}

//----------------------------------------------------------------------------
void vtkImageViewer2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderWindow:\n";
  this->RenderWindow->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Renderer:\n";
  this->Renderer->PrintSelf(os,indent.GetNextIndent());
  os << indent << "ImageActor:\n";
  this->ImageActor->PrintSelf(os,indent.GetNextIndent());
  os << indent << "WindowLevel:\n" << endl;
  this->WindowLevel->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Slice: " << this->Slice << endl;
  os << indent << "SliceOrientation: " << this->SliceOrientation << endl;
  os << indent << "InteractorStyle: " << endl;
  if (this->InteractorStyle)
  {
    os << "\n";
    this->InteractorStyle->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << "None";
  }
}
