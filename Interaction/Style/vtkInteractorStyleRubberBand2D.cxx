/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBand2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkInteractorStyleRubberBand2D.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkInteractorStyleRubberBand2D);

//--------------------------------------------------------------------------
vtkInteractorStyleRubberBand2D::vtkInteractorStyleRubberBand2D()
{
  this->PixelArray = vtkUnsignedCharArray::New();
  this->Interaction = NONE;
  this->RenderOnMouseMove = false;
  this->StartPosition[0] = 0.0;
  this->StartPosition[1] = 0.0;
  this->EndPosition[0] = 0.0;
  this->EndPosition[1] = 0.0;
}

//--------------------------------------------------------------------------
vtkInteractorStyleRubberBand2D::~vtkInteractorStyleRubberBand2D()
{
  this->PixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnLeftButtonDown()
{
  if(this->Interaction == NONE)
    {
    this->Interaction = SELECTING;
    vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();

    this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
    this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
    this->EndPosition[0] = this->StartPosition[0];
    this->EndPosition[1] = this->StartPosition[1];

    this->PixelArray->Initialize();
    this->PixelArray->SetNumberOfComponents(4);
    int *size = renWin->GetSize();
    this->PixelArray->SetNumberOfTuples(size[0]*size[1]);

    renWin->GetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, 1, this->PixelArray);
    this->FindPokedRenderer(this->StartPosition[0], this->StartPosition[1]);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnLeftButtonUp()
{
  if(this->Interaction == SELECTING)
    {
    this->Interaction = NONE;

    // Clear the rubber band
    int* size = this->Interactor->GetRenderWindow()->GetSize();
    unsigned char* pixels = this->PixelArray->GetPointer(0);
    this->Interactor->GetRenderWindow()->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, pixels, 0);
    this->Interactor->GetRenderWindow()->Frame();

    unsigned int rect[5];
    rect[0] = this->StartPosition[0];
    rect[1] = this->StartPosition[1];
    rect[2] = this->EndPosition[0];
    rect[3] = this->EndPosition[1];
    if (this->Interactor->GetShiftKey())
      {
      rect[4] = SELECT_UNION;
      }
    else
      {
      rect[4] = SELECT_NORMAL;
      }
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(rect));
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnMiddleButtonDown()
{
  if(this->Interaction == NONE)
    {
    this->Interaction = PANNING;
    this->FindPokedRenderer(
      this->Interactor->GetEventPosition()[0],
      this->Interactor->GetEventPosition()[1]);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnMiddleButtonUp()
{
  if(this->Interaction == PANNING)
    {
    this->Interaction = NONE;
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnRightButtonDown()
{
  if(this->Interaction == NONE)
    {
    this->Interaction = ZOOMING;
    this->FindPokedRenderer(
      this->Interactor->GetEventPosition()[0],
      this->Interactor->GetEventPosition()[1]);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnRightButtonUp()
{
  if(this->Interaction == ZOOMING)
    {
    this->Interaction = NONE;
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnMouseMove()
{
  if (this->Interaction == PANNING || this->Interaction == ZOOMING)
    {
    vtkRenderWindowInteractor* rwi = this->GetInteractor();
    int lastPt[] = {0, 0};
    rwi->GetLastEventPosition(lastPt);
    int curPt[] = {0, 0};
    rwi->GetEventPosition(curPt);

    vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    double lastScale = 2.0 * camera->GetParallelScale() / this->CurrentRenderer->GetSize()[1];
    double lastFocalPt[] = {0, 0, 0};
    camera->GetFocalPoint(lastFocalPt);
    double lastPos[] = {0, 0, 0};
    camera->GetPosition(lastPos);

    if (this->Interaction == PANNING)
      {
      double delta[] = {0, 0, 0};
      delta[0] = -lastScale*(curPt[0] - lastPt[0]);
      delta[1] = -lastScale*(curPt[1] - lastPt[1]);
      delta[2] = 0;
      camera->SetFocalPoint(lastFocalPt[0] + delta[0], lastFocalPt[1] + delta[1], lastFocalPt[2] + delta[2]);
      camera->SetPosition(lastPos[0] + delta[0], lastPos[1] + delta[1], lastPos[2] + delta[2]);
      this->InvokeEvent(vtkCommand::InteractionEvent);
      rwi->Render();
      }
    else if (this->Interaction == ZOOMING)
      {
      double motion = 10.0;
      double dyf = motion*(curPt[1] - lastPt[1])/this->CurrentRenderer->GetCenter()[1];
      double factor = pow(1.1, dyf);
      camera->SetParallelScale(camera->GetParallelScale() / factor);
      this->InvokeEvent(vtkCommand::InteractionEvent);
      rwi->Render();
      }
    }
  else if (this->Interaction == SELECTING)
    {
    this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
    this->EndPosition[1] = this->Interactor->GetEventPosition()[1];
    int *size = this->Interactor->GetRenderWindow()->GetSize();
    if (this->EndPosition[0] > (size[0]-1))
      {
      this->EndPosition[0] = size[0]-1;
      }
    if (this->EndPosition[0] < 0)
      {
      this->EndPosition[0] = 0;
      }
    if (this->EndPosition[1] > (size[1]-1))
      {
      this->EndPosition[1] = size[1]-1;
      }
    if (this->EndPosition[1] < 0)
      {
      this->EndPosition[1] = 0;
      }
    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->RedrawRubberBand();
    }
  else if (this->RenderOnMouseMove)
    {
    this->GetInteractor()->Render();
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnMouseWheelForward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0],
    this->Interactor->GetEventPosition()[1]);
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (!camera)
    {
    return;
    }
  this->Interaction = ZOOMING;
  double motion = 10.0;
  double dyf = motion * 0.2;
  double factor = pow(1.1, dyf);
  camera->SetParallelScale(camera->GetParallelScale() / factor);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->GetInteractor()->Render();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::OnMouseWheelBackward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0],
    this->Interactor->GetEventPosition()[1]);
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (!camera)
    {
    return;
    }
  this->Interaction = ZOOMING;
  double motion = 10.0;
  double dyf = motion * -0.2;
  double factor = pow(1.1, dyf);
  camera->SetParallelScale(camera->GetParallelScale() / factor);
  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->GetInteractor()->Render();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::RedrawRubberBand()
{
  // Update the rubber band on the screen
  int *size = this->Interactor->GetRenderWindow()->GetSize();

  vtkUnsignedCharArray *tmpPixelArray = vtkUnsignedCharArray::New();
  tmpPixelArray->DeepCopy(this->PixelArray);
  unsigned char *pixels = tmpPixelArray->GetPointer(0);

  int min[2], max[2];

  min[0] = this->StartPosition[0] <= this->EndPosition[0] ?
    this->StartPosition[0] : this->EndPosition[0];
  if (min[0] < 0) { min[0] = 0; }
  if (min[0] >= size[0]) { min[0] = size[0] - 1; }

  min[1] = this->StartPosition[1] <= this->EndPosition[1] ?
    this->StartPosition[1] : this->EndPosition[1];
  if (min[1] < 0) { min[1] = 0; }
  if (min[1] >= size[1]) { min[1] = size[1] - 1; }

  max[0] = this->EndPosition[0] > this->StartPosition[0] ?
    this->EndPosition[0] : this->StartPosition[0];
  if (max[0] < 0) { max[0] = 0; }
  if (max[0] >= size[0]) { max[0] = size[0] - 1; }

  max[1] = this->EndPosition[1] > this->StartPosition[1] ?
    this->EndPosition[1] : this->StartPosition[1];
  if (max[1] < 0) { max[1] = 0; }
  if (max[1] >= size[1]) { max[1] = size[1] - 1; }

  int i;
  for (i = min[0]; i <= max[0]; i++)
    {
    pixels[4*(min[1]*size[0]+i)] = 255 ^ pixels[4*(min[1]*size[0]+i)];
    pixels[4*(min[1]*size[0]+i)+1] = 255 ^ pixels[4*(min[1]*size[0]+i)+1];
    pixels[4*(min[1]*size[0]+i)+2] = 255 ^ pixels[4*(min[1]*size[0]+i)+2];
    pixels[4*(max[1]*size[0]+i)] = 255 ^ pixels[4*(max[1]*size[0]+i)];
    pixels[4*(max[1]*size[0]+i)+1] = 255 ^ pixels[4*(max[1]*size[0]+i)+1];
    pixels[4*(max[1]*size[0]+i)+2] = 255 ^ pixels[4*(max[1]*size[0]+i)+2];
    }
  for (i = min[1]+1; i < max[1]; i++)
    {
    pixels[4*(i*size[0]+min[0])] = 255 ^ pixels[4*(i*size[0]+min[0])];
    pixels[4*(i*size[0]+min[0])+1] = 255 ^ pixels[4*(i*size[0]+min[0])+1];
    pixels[4*(i*size[0]+min[0])+2] = 255 ^ pixels[4*(i*size[0]+min[0])+2];
    pixels[4*(i*size[0]+max[0])] = 255 ^ pixels[4*(i*size[0]+max[0])];
    pixels[4*(i*size[0]+max[0])+1] = 255 ^ pixels[4*(i*size[0]+max[0])+1];
    pixels[4*(i*size[0]+max[0])+2] = 255 ^ pixels[4*(i*size[0]+max[0])+2];
    }

  this->Interactor->GetRenderWindow()->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, pixels, 0);
  this->Interactor->GetRenderWindow()->Frame();

  tmpPixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interaction: " << this->Interaction << endl;
  os << indent << "RenderOnMouseMove: " << this->RenderOnMouseMove << endl;
  os << indent << "StartPosition: " << this->StartPosition[0] << "," << this->StartPosition[1] << endl;
  os << indent << "EndPosition: " << this->EndPosition[0] << "," << this->EndPosition[1] << endl;
}
