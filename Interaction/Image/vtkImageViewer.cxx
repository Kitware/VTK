/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageViewer.h"

#include "vtkActor2D.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageViewer);

//----------------------------------------------------------------------------
vtkImageViewer::vtkImageViewer()
{
  this->RenderWindow = vtkRenderWindow::New();
  this->Renderer     = vtkRenderer::New();
  this->ImageMapper  = vtkImageMapper::New();
  this->Actor2D      = vtkActor2D::New();

  // setup the pipeline
  this->Actor2D->SetMapper(this->ImageMapper);
  this->Renderer->AddActor2D(this->Actor2D);
  this->RenderWindow->AddRenderer(this->Renderer);

  this->FirstRender = 1;

  this->Interactor = 0;
  this->InteractorStyle = 0;
}


//----------------------------------------------------------------------------
vtkImageViewer::~vtkImageViewer()
{
  this->ImageMapper->Delete();
  this->Actor2D->Delete();
  this->RenderWindow->Delete();
  this->Renderer->Delete();

  if (this->Interactor)
    {
    this->Interactor->Delete();
    }
  if (this->InteractorStyle)
    {
    this->InteractorStyle->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageMapper:\n";
  this->ImageMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RenderWindow:\n";
  this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Renderer:\n";
  this->Renderer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Actor2D:\n";
  this->Actor2D->PrintSelf(os, indent.GetNextIndent());
}



//----------------------------------------------------------------------------
void vtkImageViewer::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}
//----------------------------------------------------------------------------
void vtkImageViewer::SetPosition(int a[2])
{
  this->SetPosition(a[0],a[1]);
}

//----------------------------------------------------------------------------
class vtkImageViewerCallback : public vtkCommand
{
public:
  static vtkImageViewerCallback *New() { return new vtkImageViewerCallback; }

  void Execute(vtkObject *caller,
               unsigned long event,
               void *vtkNotUsed(callData))
    {
      if (this->IV->GetInput() == NULL)
        {
        return;
        }

      // Reset

      if (event == vtkCommand::ResetWindowLevelEvent)
        {
        this->IV->GetInputAlgorithm()->UpdateInformation();
        vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
          this->IV->GetInputAlgorithm()->GetOutputInformation(0),
          vtkStreamingDemandDrivenPipeline::GetWholeExtent(
            this->IV->GetInputAlgorithm()->GetOutputInformation(0)));
        this->IV->GetInputAlgorithm()->Update();
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

      double dx = 4.0 * (isi->GetWindowLevelCurrentPosition()[0] -
                        isi->GetWindowLevelStartPosition()[0]) / size[0];
      double dy = 4.0 * (isi->GetWindowLevelStartPosition()[1] -
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

  vtkImageViewer *IV;
  double InitialWindow;
  double InitialLevel;
};

//----------------------------------------------------------------------------
void vtkImageViewer::SetupInteractor(vtkRenderWindowInteractor *rwi)
{
  if (this->Interactor && rwi != this->Interactor)
    {
    this->Interactor->Delete();
    }
  if (!this->InteractorStyle)
    {
    this->InteractorStyle = vtkInteractorStyleImage::New();
    vtkImageViewerCallback *cbk = vtkImageViewerCallback::New();
    cbk->IV = this;
    this->InteractorStyle->AddObserver(vtkCommand::WindowLevelEvent, cbk);
    this->InteractorStyle->AddObserver(vtkCommand::StartWindowLevelEvent, cbk);
    this->InteractorStyle->AddObserver(vtkCommand::ResetWindowLevelEvent, cbk);
    cbk->Delete();
    }

  if (!this->Interactor)
    {
    this->Interactor = rwi;
    rwi->Register(this);
    }
  this->Interactor->SetInteractorStyle(this->InteractorStyle);
  this->Interactor->SetRenderWindow(this->RenderWindow);
}

//----------------------------------------------------------------------------
void vtkImageViewer::Render()
{
  if (this->FirstRender)
    {
    // initialize the size if not set yet
    if (this->RenderWindow->GetSize()[0] == 0 && this->ImageMapper->GetInput())
      {
      // get the size from the mappers input
      this->ImageMapper->GetInputAlgorithm()->UpdateInformation();
      int *ext = this->ImageMapper->GetInputInformation()->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      // if it would be smaller than 150 by 100 then limit to 150 by 100
      int xs = ext[1] - ext[0] + 1;
      int ys = ext[3] - ext[2] + 1;
      this->RenderWindow->SetSize(xs < 150 ? 150 : xs,
                                  ys < 100 ? 100 : ys);
      }
    this->FirstRender = 0;
    }
  this->RenderWindow->Render();
}

//----------------------------------------------------------------------------
void vtkImageViewer::SetOffScreenRendering(int i)
{
  this->RenderWindow->SetOffScreenRendering(i);
}

//----------------------------------------------------------------------------
int vtkImageViewer::GetOffScreenRendering()
{
  return this->RenderWindow->GetOffScreenRendering();
}

//----------------------------------------------------------------------------
void vtkImageViewer::OffScreenRenderingOn()
{
  this->SetOffScreenRendering(1);
}

//----------------------------------------------------------------------------
void vtkImageViewer::OffScreenRenderingOff()
{
  this->SetOffScreenRendering(0);
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkImageViewer::GetInputAlgorithm()
{
  return this->ImageMapper->GetInputAlgorithm();
}
