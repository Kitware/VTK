/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper.cxx
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

#include "vtkImageMapper.h"

#include "vtkActor2D.h"
#include "vtkImager.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkImageData.h"
#include "vtkImagingFactory.h"

vtkCxxRevisionMacro(vtkImageMapper, "1.42");

#define VTK_RINT(x) ((x > 0.0) ? (int)(x + 0.5) : (int)(x - 0.5))

vtkImageMapper::vtkImageMapper()
{
  vtkDebugMacro(<< "vtkImageMapper::vtkImageMapper" );

  this->NumberOfRequiredInputs = 1;

  this->ColorWindow = 2000;
  this->ColorLevel  = 1000;

  this->DisplayExtent[0] = this->DisplayExtent[1] = 0;
  this->DisplayExtent[2] = this->DisplayExtent[3] = 0;
  this->DisplayExtent[4] = this->DisplayExtent[5] = 0;
  this->ZSlice = 0;

  this->RenderToRectangle = 0;
  this->UseCustomExtents  = 0;
  this->CustomDisplayExtents[0] = this->CustomDisplayExtents[1] = 0;
  this->CustomDisplayExtents[2] = this->CustomDisplayExtents[3] = 0;

}

vtkImageMapper::~vtkImageMapper()
{
}

//----------------------------------------------------------------------------
void vtkImageMapper::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMapper::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

unsigned long int vtkImageMapper::GetMTime()
{
  unsigned long mTime=this->vtkMapper2D::GetMTime();
  return mTime;
}

void vtkImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Color Window: " << this->ColorWindow << "\n";
  os << indent << "Color Level: " << this->ColorLevel << "\n";
  os << indent << "ZSlice: " << this->ZSlice << "\n";
  os << indent << "RenderToRectangle: " << this->RenderToRectangle << "\n";
  os << indent << "UseCustomExtents: " << this->UseCustomExtents << "\n";
  os << indent << "CustomDisplayExtents: " <<
    this->CustomDisplayExtents[0] << " " <<
    this->CustomDisplayExtents[1] << " " <<
    this->CustomDisplayExtents[2] << " " <<
    this->CustomDisplayExtents[3] << "\n";
  //
}

vtkImageMapper* vtkImageMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkImagingFactory::CreateInstance("vtkImageMapper");
  return (vtkImageMapper*)ret;
}

float vtkImageMapper::GetColorShift()
{
  return this->ColorWindow /2.0 - this->ColorLevel;
}

float vtkImageMapper::GetColorScale()
{
  return 255.0 / this->ColorWindow;
}

void vtkImageMapper::RenderStart(vtkViewport* viewport, vtkActor2D* actor)
{
  vtkDebugMacro(<< "vtkImageMapper::RenderOverlay");

  vtkImageData *data;

  if (!viewport)
    {
    vtkErrorMacro (<< "vtkImageMapper::Render - Null viewport argument");
    return;
    }

  if (!actor)
    {
    vtkErrorMacro (<<"vtkImageMapper::Render - Null actor argument");
    return;    
    }


  if (this->NumberOfInputs < 1)
    {
    vtkDebugMacro(<< "vtkImageMapper::Render - Please Set the input.");
    return;
    }

  this->GetInput()->UpdateInformation();

  if (!this->UseCustomExtents)
    {
    // start with the wholeExtent
    int wholeExtent[6];
    memcpy(wholeExtent,this->GetInput()->GetWholeExtent(),6*sizeof(int));
    memcpy(this->DisplayExtent,this->GetInput()->GetWholeExtent(),6*sizeof(int));
    // Set The z values to the zslice
    this->DisplayExtent[4] = this->ZSlice;
    this->DisplayExtent[5] = this->ZSlice;

    // scale currently not handled
    //float *scale = actor->GetScale();

    // get the position
    int *pos = actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

    // Get the viewport coordinates
    float vCoords[4];
    vCoords[0] = 0.0;
    vCoords[1] = 0.0;
    vCoords[2] = 1.0;
    vCoords[3] = 1.0;
    viewport->NormalizedViewportToViewport(vCoords[0],vCoords[1]);
    viewport->NormalizedViewportToViewport(vCoords[2],vCoords[3]);
    int *vSize = viewport->GetSize();

    // the basic formula is that the draw pos equals
    // the pos + extentPos + clippedAmount
    // The concrete subclass will get the pos in display
    // coordinates so we need to provide the extentPos plus
    // clippedAmount in the PositionAdjustment variable

    // Now clip to imager extents
    if (pos[0] + wholeExtent[0] < 0)
      {
      this->DisplayExtent[0] = -pos[0];
      }
    if ((pos[0]+wholeExtent[1]) > vSize[0])
      {
      this->DisplayExtent[1] = vSize[0] - pos[0];
      }
    if (pos[1] + wholeExtent[2] < 0)
      {
      this->DisplayExtent[2] = -pos[1];
      }
    if ((pos[1]+wholeExtent[3]) > vSize[1])
      {
      this->DisplayExtent[3] = vSize[1] - pos[1];
      }

    // check for the condition where no pixels are visible.
    if (this->DisplayExtent[0] > wholeExtent[1] ||
      this->DisplayExtent[1] < wholeExtent[0] ||
      this->DisplayExtent[2] > wholeExtent[3] ||
      this->DisplayExtent[3] < wholeExtent[2] ||
      this->DisplayExtent[4] > wholeExtent[5] ||
      this->DisplayExtent[5] < wholeExtent[4])
      {
      return;
      }

    this->GetInput()->SetUpdateExtent(this->DisplayExtent);

    // set the position adjustment
    this->PositionAdjustment[0] = this->DisplayExtent[0];
    this->PositionAdjustment[1] = this->DisplayExtent[2];
    }
  else // UseCustomExtents
    {
    this->DisplayExtent[0] = this->CustomDisplayExtents[0];
    this->DisplayExtent[1] = this->CustomDisplayExtents[1];
    this->DisplayExtent[2] = this->CustomDisplayExtents[2];
    this->DisplayExtent[3] = this->CustomDisplayExtents[3];
    this->DisplayExtent[4] = this->ZSlice;
    this->DisplayExtent[5] = this->ZSlice;
    //
    this->GetInput()->SetUpdateExtentToWholeExtent();
    // clear the position adjustment
    this->PositionAdjustment[0] = 0;
    this->PositionAdjustment[1] = 0;
    }

  // Get the region from the input
  this->GetInput()->Update();
  data = this->GetInput();
  if ( !data)
    {
    vtkErrorMacro(<< "Render: Could not get data from input.");
    return;
    }

  this->RenderData(viewport, data, actor);
}

//----------------------------------------------------------------------------
int vtkImageMapper::GetWholeZMin()
{
  int *extent;
  
  if ( ! this->GetInput())
    {
    return 0;
    }
  this->GetInput()->UpdateInformation();
  extent = this->GetInput()->GetWholeExtent();
  return extent[4];
}

//----------------------------------------------------------------------------
int vtkImageMapper::GetWholeZMax()
{
  int *extent;
  
  if ( ! this->GetInput())
    {
    return 0;
    }
  this->GetInput()->UpdateInformation();
  extent = this->GetInput()->GetWholeExtent();
  return extent[5];
}


