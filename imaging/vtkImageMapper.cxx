/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkImageMapper.h"
#include "vtkImageCache.h"

#ifdef _WIN32
  #include "vtkWin32ImageMapper.h"
#else
  #include "vtkXImageMapper.h"
#endif

#include "vtkActor2D.h"
#include "vtkImager.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkImageData.h"

vtkImageMapper::vtkImageMapper()
{
  vtkDebugMacro(<< "vtkImageMapper::vtkImageMapper" );

  this->Input = (vtkImageCache*) NULL;

  //this->ColorWindow = 255.0;
  //this->ColorLevel = 127.0;

  this->ColorWindow = 2000;
  this->ColorLevel = 1000;

  this->ZSlice = 0;
}

void vtkImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkMapper2D::PrintSelf(os, indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Color Window: " << this->ColorWindow << "\n";
  os << indent << "Color Level: " << this->ColorLevel << "\n";
  os << indent << "ZSlice: " << this->ZSlice << "\n";
}

vtkImageMapper* vtkImageMapper::New()
{
  #ifdef _WIN32
    return vtkWin32ImageMapper::New();
  #else
    return vtkXImageMapper::New();
  #endif

}

float vtkImageMapper::GetColorShift()
{
  return this->ColorWindow /2.0 - this->ColorLevel;
}

float vtkImageMapper::GetColorScale()
{
  return 255.0 / this->ColorWindow;
}

void vtkImageMapper::Render(vtkViewport* viewport, vtkActor2D* actor)
{
  vtkDebugMacro(<< "vtkImageMapper::Render");

  vtkImageData *data;
  int displayExtent[6];
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

  this->Input->UpdateImageInformation();
  // start with the wholeExtent
  memcpy(wholeExtent,this->Input->GetWholeExtent(),6*sizeof(int));
  memcpy(displayExtent,this->Input->GetWholeExtent(),6*sizeof(int));

  // Set The z values to the zslice
  displayExtent[4] = this->ZSlice;
  displayExtent[5] = this->ZSlice;

  // scale currently not handled
  //float *scale = actor->GetScale();

  // get the position
  int *pos = actor->GetComputedViewportPixelPosition(viewport);
  
  // Get the viewport coordinates
  float* vpt = viewport->GetViewport(); 

  // Get the window size
  vtkWindow* window = viewport->GetVTKWindow();
  int* winSize = window->GetSize();

  // if we clip based on 0,0 then we need to store the amount we
  // adjusted by so that we can take that into account in the
  // positioning of the actor
  // initialize PositionAdjustment to zero
  this->PositionAdjustment[0] = 0;
  this->PositionAdjustment[1] = 0;

  // Now clip to imager extents
  if (pos[0] < 0) 
    {
    this->PositionAdjustment[0] = -1*pos[0];
    displayExtent[0] -= pos[0];
    }
  if ((pos[0]+wholeExtent[1] - wholeExtent[0] + 1) > 
      (winSize[0] - 1)*(vpt[XMAX]-vpt[XMIN])) 
    {
    displayExtent[1] = displayExtent[1] - 
      (pos[0]+wholeExtent[1] - wholeExtent[0] + 1) + 
      (winSize[0] - 1)*(vpt[XMAX]-vpt[XMIN]);
    }
  if (pos[1] < 0) 
    {
    this->PositionAdjustment[1] = -1*pos[1];
    displayExtent[2] -= pos[1];
    }
  if ((pos[1] + wholeExtent[3] - wholeExtent[2] + 1) > 
      (winSize[1] - 1)*(vpt[YMAX]-vpt[YMIN])) 
    {
    displayExtent[3] = displayExtent[3] - 
      (pos[1]+wholeExtent[3] - wholeExtent[2] + 1) + 
      (winSize[1] - 1)*(vpt[YMAX]-vpt[YMIN]);
    }

  this->Input->SetUpdateExtent(displayExtent);

  // Get the region from the input
  data = this->Input->UpdateAndReturnData();
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
  this->Input->UpdateImageInformation();
  extent = this->Input->GetWholeExtent();
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
  this->Input->UpdateImageInformation();
  extent = this->Input->GetWholeExtent();
  return extent[5];
}


