/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkImageMapper.h"


#include "vtkActor2D.h"
#include "vtkImager.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkImageData.h"
#include "vtkImagingFactory.h"

#define VTK_RINT(x) ((x > 0.0) ? (int)(x + 0.5) : (int)(x - 0.5))

vtkImageMapper::vtkImageMapper()
{
  vtkDebugMacro(<< "vtkImageMapper::vtkImageMapper" );

  this->Input = NULL;

  //this->ColorWindow = 255.0;
  //this->ColorLevel = 127.0;

  this->ColorWindow = 2000;
  this->ColorLevel = 1000;

  this->DisplayExtent[0] = this->DisplayExtent[1] = 0;
  this->DisplayExtent[2] = this->DisplayExtent[3] = 0;
  this->DisplayExtent[4] = this->DisplayExtent[5] = 0;
  this->ZSlice = 0;
}

vtkImageMapper::~vtkImageMapper()
{
  if (this->Input)
    {
    this->GetInput()->UnRegister(this);
    this->Input = NULL;
    }
}

void vtkImageMapper::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  this->vtkMapper2D::PrintSelf(os, indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Color Window: " << this->ColorWindow << "\n";
  os << indent << "Color Level: " << this->ColorLevel << "\n";
  os << indent << "ZSlice: " << this->ZSlice << "\n";
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
  int wholeExtent[6];

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


  if (!this->Input)
    {
    vtkDebugMacro(<< "vtkImageMapper::Render - Please Set the input.");
    return;
    }

  this->GetInput()->UpdateInformation();
  // start with the wholeExtent
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
  int vSize[2];
  // size excludes last pixel except for last pixelof window
  // this is to prevent overlapping viewports
  vSize[0] = VTK_RINT(vCoords[2]) - VTK_RINT(vCoords[0]);
  vSize[1] = VTK_RINT(vCoords[3]) - VTK_RINT(vCoords[1]);
  viewport->ViewportToNormalizedDisplay(vCoords[2],vCoords[3]);
  if (vCoords[2] == 1.0) 
    {
    vSize[0]++;
    }
  if (vCoords[3] == 1.0)
    {
    vSize[1]++;
    }
  
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
  
  if ( ! this->Input)
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
  
  if ( ! this->Input)
    {
    return 0;
    }
  this->GetInput()->UpdateInformation();
  extent = this->GetInput()->GetWholeExtent();
  return extent[5];
}


