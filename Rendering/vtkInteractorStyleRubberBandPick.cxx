/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBandPick.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleRubberBandPick.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkAbstractPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkAreaPicker.h"

vtkCxxRevisionMacro(vtkInteractorStyleRubberBandPick, "1.2");
vtkStandardNewMacro(vtkInteractorStyleRubberBandPick);

#define VTKISRBP_ORIENT 0
#define VTKISRBP_SELECT 1

//--------------------------------------------------------------------------
vtkInteractorStyleRubberBandPick::vtkInteractorStyleRubberBandPick()
{
  this->CurrentMode = VTKISRBP_ORIENT;
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->Moving = 0;
  this->PixelArray = vtkUnsignedCharArray::New();
}

//--------------------------------------------------------------------------
vtkInteractorStyleRubberBandPick::~vtkInteractorStyleRubberBandPick()
{
  this->PixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::StartSelect()
{
  this->CurrentMode = VTKISRBP_SELECT;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::OnChar()
{
  switch (this->Interactor->GetKeyCode())
    {
    case 'r':
    case 'R':
      //r toggles the rubber band selection mode for mouse button 1
      if (this->CurrentMode == VTKISRBP_ORIENT)
        {
        this->CurrentMode = VTKISRBP_SELECT;
        }
      else
        { 
        this->CurrentMode = VTKISRBP_ORIENT;
        }
      break;
    case 'p' :
    case 'P' :
    {
      // p fires a pick event for the 3x3 window around the current mouse coords
      vtkRenderWindowInteractor *rwi = this->Interactor;
      int *eventPos = rwi->GetEventPosition();
      this->FindPokedRenderer(eventPos[0], eventPos[1]);
      this->StartPosition[0] = eventPos[0]-1;
      this->StartPosition[1] = eventPos[1]-1;
      this->EndPosition[0] = eventPos[0]+1;
      this->EndPosition[1] = eventPos[1]+1;
      this->Pick();
      break;
    }
    default:
      this->Superclass::OnChar();
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::OnLeftButtonDown()
{
  if (this->CurrentMode != VTKISRBP_SELECT)
    {
    //if not in rubber band mode, let the parent class handle it
    this->Superclass::OnLeftButtonDown();
    return;
    }

  if (!this->Interactor)
    {
    return;
    }

  //otherwise record the rubber band starting coordinate

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

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::OnMouseMove()
{
  if (this->CurrentMode != VTKISRBP_SELECT)
    {
    //if not in rubber band mode,  let the parent class handle it
    this->Superclass::OnMouseMove();
    return;
    }

  if (!this->Interactor || !this->Moving)
    {
    return;
    }

  //otherwise update the rubber band on the screen
  
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
  
  tmpPixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::OnLeftButtonUp()
{
  if (this->CurrentMode != VTKISRBP_SELECT)
    {
    //if not in rubber band mode,  let the parent class handle it
    this->Superclass::OnLeftButtonUp();
    return;
    }

  if (!this->Interactor || !this->Moving)
    {
    return;
    }

  //otherwise record the rubber band end coordinate and then fire off a pick
  if (   (this->StartPosition[0] != this->EndPosition[0])
      || (this->StartPosition[1] != this->EndPosition[1]) )
    {
    this->Pick();
    }
  this->Moving = 0;
  this->CurrentMode = VTKISRBP_ORIENT;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::Pick()
{
  //find rubber band lower left, upper right and center
  int width, height;
  width = abs(this->EndPosition[0] - this->StartPosition[0]);
  height = abs(this->EndPosition[1] - this->StartPosition[1]);
  
  double min[2];
  double rbcenter[3];
  min[0] = this->StartPosition[0] < this->EndPosition[0] ?
    this->StartPosition[0] : this->EndPosition[0];
  min[1] = this->StartPosition[1] < this->EndPosition[1] ?
    this->StartPosition[1] : this->EndPosition[1];
  double max[2];
  max[0] = this->StartPosition[0] > this->EndPosition[0] ?
    this->StartPosition[0] : this->EndPosition[0];
  max[1] = this->StartPosition[1] > this->EndPosition[1] ?
    this->StartPosition[1] : this->EndPosition[1];

  rbcenter[0] = min[0] + 0.5*width;
  rbcenter[1] = min[1] + 0.5*height;
  rbcenter[2] = 0;
  
  if (this->State == VTKIS_NONE) 
    {
    //tell the RenderWindowInteractor's picker to make it happen
    vtkRenderWindowInteractor *rwi = this->Interactor;

    vtkAssemblyPath *path = NULL;
    rwi->StartPickCallback();
    vtkAbstractPropPicker *picker = 
      vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker());
    if ( picker != NULL )
      {
      vtkAreaPicker *areaPicker = vtkAreaPicker::SafeDownCast(picker);
      if (areaPicker != NULL)
        {
        areaPicker->AreaPick(min[0], min[1], max[0], max[1], 
                             this->CurrentRenderer);
        }
      else
        {
        picker->Pick(rbcenter[0], rbcenter[1], 
                     0.0, this->CurrentRenderer);
        }
      path = picker->GetPath();
      }
    if ( path == NULL )
      {
      this->HighlightProp(NULL);
      this->PropPicked = 0;
      }
    else
      {
      //highlight the one prop that the picker saved in the path
      this->HighlightProp(path->GetFirstNode()->GetViewProp());
      this->PropPicked = 1;
      }
    rwi->EndPickCallback();
    }
  
  this->Interactor->Render();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleRubberBandPick::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
