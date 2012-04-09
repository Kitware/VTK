/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBand3D.cxx

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

#include "vtkInteractorStyleRubberBand3D.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkInteractorStyleRubberBand3D);

//--------------------------------------------------------------------------
vtkInteractorStyleRubberBand3D::vtkInteractorStyleRubberBand3D()
{
  this->PixelArray = vtkUnsignedCharArray::New();
  this->Interaction = NONE;
  this->RenderOnMouseMove = false;
}

//--------------------------------------------------------------------------
vtkInteractorStyleRubberBand3D::~vtkInteractorStyleRubberBand3D()
{
  this->PixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::OnLeftButtonDown()
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
void vtkInteractorStyleRubberBand3D::OnLeftButtonUp()
{
  if(this->Interaction == SELECTING)
    {
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
    this->Interaction = NONE;
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::OnMiddleButtonDown()
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
void vtkInteractorStyleRubberBand3D::OnMiddleButtonUp()
{
  if(this->Interaction == PANNING)
    {
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->Interaction = NONE;
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::OnRightButtonDown()
{
  if(this->Interaction == NONE)
    {
    if (this->Interactor->GetShiftKey())
      {
      this->Interaction = ZOOMING;
      }
    else
      {
      this->Interaction = ROTATING;
      }
    this->FindPokedRenderer(
      this->Interactor->GetEventPosition()[0], 
      this->Interactor->GetEventPosition()[1]);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::OnRightButtonUp()
{
  if(this->Interaction == ZOOMING || this->Interaction == ROTATING)
    {
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->Interaction = NONE;
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::OnMouseMove()
{
  if (this->Interaction == PANNING)
    {
    this->Superclass::Pan();
    }
  else if (this->Interaction == ZOOMING)
    {
    this->Superclass::Dolly();
    }
  else if (this->Interaction == ROTATING)
    {
    this->Superclass::Rotate();
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
void vtkInteractorStyleRubberBand3D::OnMouseWheelForward()
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
  this->Superclass::OnMouseWheelForward();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::OnMouseWheelBackward()
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
  this->Superclass::OnMouseWheelBackward();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBand3D::RedrawRubberBand()
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
void vtkInteractorStyleRubberBand3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interaction: " << this->Interaction << endl;
  os << indent << "RenderOnMouseMove: " << this->RenderOnMouseMove << endl;
  os << indent << "StartPosition: " << this->StartPosition[0] << "," << this->StartPosition[1] << endl;
  os << indent << "EndPosition: " << this->EndPosition[0] << "," << this->EndPosition[1] << endl;
}
