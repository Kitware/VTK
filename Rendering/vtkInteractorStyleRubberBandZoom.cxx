/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBandZoom.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleRubberBandZoom.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkInteractorStyleRubberBandZoom, "1.2");
vtkStandardNewMacro(vtkInteractorStyleRubberBandZoom);

vtkInteractorStyleRubberBandZoom::vtkInteractorStyleRubberBandZoom()
{
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->Moving = 0;
  this->PixelArray = vtkUnsignedCharArray::New();
}

vtkInteractorStyleRubberBandZoom::~vtkInteractorStyleRubberBandZoom()
{
  this->PixelArray->Delete();
}

void vtkInteractorStyleRubberBandZoom::OnMouseMove()
{
  if (!this->Interactor || !this->Moving)
    {
    return;
    }
  
  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];  
  
  vtkUnsignedCharArray *tmpPixelArray = vtkUnsignedCharArray::New();
  tmpPixelArray->DeepCopy(this->PixelArray);
  
  unsigned char *pixels = tmpPixelArray->GetPointer(0);
  
  int min[2], max[2];
  min[0] = this->StartPosition[0] <= this->EndPosition[0] ?
    this->StartPosition[0] : this->EndPosition[0];
  min[1] = this->StartPosition[1] <= this->EndPosition[1] ?
    this->StartPosition[1] : this->EndPosition[1];
  max[0] = this->EndPosition[0] > this->StartPosition[0] ?
    this->EndPosition[0] : this->StartPosition[0];
  max[1] = this->EndPosition[1] > this->StartPosition[1] ?
    this->EndPosition[1] : this->StartPosition[1];

  int *size = this->Interactor->GetRenderWindow()->GetSize();
  
  int i;
  for (i = min[0]; i <= max[0]; i++)
    {
    pixels[3*(min[1]*size[0]+i)] = 255 ^ pixels[3*(min[1]*size[0]+i)];
    pixels[3*(min[1]*size[0]+i)+1] = 255 ^ pixels[3*(min[1]*size[0]+i)+1];
    pixels[3*(min[1]*size[0]+i)+2] = 255 ^ pixels[3*(min[1]*size[0]+i)+2];
    pixels[3*(max[1]*size[0]+i)] = 255 ^ pixels[3*(max[1]*size[0]+i)];
    pixels[3*(max[1]*size[0]+i)+1] = 255 ^ pixels[3*(max[1]*size[0]+i)+1];
    pixels[3*(max[1]*size[0]+i)+2] = 255 ^ pixels[3*(max[1]*size[0]+i)+2];
    }
  for (i = min[1]+1; i < max[1]; i++)
    {
    pixels[3*(i*size[0]+min[0])] = 255 ^ pixels[3*(i*size[0]+min[0])];
    pixels[3*(i*size[0]+min[0])+1] = 255 ^ pixels[3*(i*size[0]+min[0])+1];
    pixels[3*(i*size[0]+min[0])+2] = 255 ^ pixels[3*(i*size[0]+min[0])+2];
    pixels[3*(i*size[0]+max[0])] = 255 ^ pixels[3*(i*size[0]+max[0])];
    pixels[3*(i*size[0]+max[0])+1] = 255 ^ pixels[3*(i*size[0]+max[0])+1];
    pixels[3*(i*size[0]+max[0])+2] = 255 ^ pixels[3*(i*size[0]+max[0])+2];
    }
  
  this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0]-1, size[1]-1, pixels, 1);
}

void vtkInteractorStyleRubberBandZoom::OnLeftButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }
  this->Moving = 1;
  
  vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();
  
  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];
  
  this->PixelArray->Initialize();
  this->PixelArray->SetNumberOfComponents(3);
  int *size = renWin->GetSize();
  this->PixelArray->SetNumberOfTuples(size[0]*size[1]);
  
  renWin->GetPixelData(0, 0, size[0]-1, size[1]-1, 1, this->PixelArray);
  
  this->FindPokedRenderer(this->StartPosition[0], this->StartPosition[1]);
}

void vtkInteractorStyleRubberBandZoom::OnLeftButtonUp()
{
  if (!this->Interactor || !this->Moving)
    {
    return;
    }

  if (   (this->StartPosition[0] != this->EndPosition[0])
      || (this->StartPosition[1] != this->EndPosition[1]) )
    {
    this->Zoom();
    }
  this->Moving = 0;
}

void vtkInteractorStyleRubberBandZoom::Zoom()
{
  int width, height;
  width = abs(this->EndPosition[0] - this->StartPosition[0]);
  height = abs(this->EndPosition[1] - this->StartPosition[1]);
  int *size = this->CurrentRenderer->GetSize();
  int *origin = this->CurrentRenderer->GetOrigin();
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  
  int min[2];
  float rbcenter[3];
  min[0] = this->StartPosition[0] < this->EndPosition[0] ?
    this->StartPosition[0] : this->EndPosition[0];
  min[1] = this->StartPosition[1] < this->EndPosition[1] ?
    this->StartPosition[1] : this->EndPosition[1];

  rbcenter[0] = min[0] + 0.5*width;
  rbcenter[1] = min[1] + 0.5*height;
  rbcenter[2] = 0;
  
  this->CurrentRenderer->SetDisplayPoint(rbcenter);
  this->CurrentRenderer->DisplayToView();
  this->CurrentRenderer->ViewToWorld();

  float invw;
  float worldRBCenter[4];
  this->CurrentRenderer->GetWorldPoint(worldRBCenter);
  invw = 1.0f/worldRBCenter[3];
  worldRBCenter[0] *= invw;
  worldRBCenter[1] *= invw;
  worldRBCenter[2] *= invw;

  float winCenter[3];
  winCenter[0] = origin[0] + 0.5f*size[0];
  winCenter[1] = origin[1] + 0.5f*size[1];
  winCenter[2] = 0;

  this->CurrentRenderer->SetDisplayPoint(winCenter);
  this->CurrentRenderer->DisplayToView();
  this->CurrentRenderer->ViewToWorld();

  float worldWinCenter[4];
  this->CurrentRenderer->GetWorldPoint(worldWinCenter);
  invw = 1.0f/worldWinCenter[3];
  worldWinCenter[0] *= invw;
  worldWinCenter[1] *= invw;
  worldWinCenter[2] *= invw;

  float translation[3];
  translation[0] = worldRBCenter[0] - worldWinCenter[0];
  translation[1] = worldRBCenter[1] - worldWinCenter[1];
  translation[2] = worldRBCenter[2] - worldWinCenter[2];

  float pos[3], fp[3];
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);

  pos[0] += translation[0]; pos[1] += translation[1]; pos[2] += translation[2];
  fp[0] += translation[0];  fp[1] += translation[1];  fp[2] += translation[2];

  cam->SetPosition(pos);
  cam->SetFocalPoint(fp);

  if (width > height)
    {
    cam->Zoom(size[0] / (float)width);
    }
  else
    {
    cam->Zoom(size[1] / (float)height);
    }
  
  this->Interactor->Render();
}

void vtkInteractorStyleRubberBandZoom::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
