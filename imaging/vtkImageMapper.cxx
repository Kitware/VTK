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

  this->DisplayExtent[0] = -VTK_LARGE_INTEGER;
  this->DisplayExtent[1] = VTK_LARGE_INTEGER;
  this->DisplayExtent[2] = -VTK_LARGE_INTEGER;
  this->DisplayExtent[3] = VTK_LARGE_INTEGER;
  this->DisplayExtent[4] = -VTK_LARGE_INTEGER;
  this->DisplayExtent[5] = VTK_LARGE_INTEGER;
  
}

void vtkImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkMapper2D::PrintSelf(os, indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Color Window: " << this->ColorWindow << "\n";
  os << indent << "Color Level: " << this->ColorLevel << "\n";
  os << indent << "Display Extent: (" << this->DisplayExtent[0] << ", "
     << this->DisplayExtent[1] << ", " << this->DisplayExtent[2] << ", " 
     << this->DisplayExtent[3] << ", " << this->DisplayExtent[4]
     << ", " << this->DisplayExtent [5] << ")\n";
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

  int min, max;
  int wholeMin, wholeMax;
  int idx, axis;
  vtkImageData *data;
  int displayExtent[6];

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
  int *wholeExtent = this->Input->GetWholeExtent();

  displayExtent[0] = -VTK_LARGE_INTEGER;
  displayExtent[1] = VTK_LARGE_INTEGER;
  displayExtent[2] = -VTK_LARGE_INTEGER;
  displayExtent[3] = VTK_LARGE_INTEGER;
  displayExtent[4] = -VTK_LARGE_INTEGER;
  displayExtent[5] = VTK_LARGE_INTEGER;

  // Clip minimum extents
  for (idx = 0; idx < 3; idx++)
    {
    if (displayExtent[idx*2] < wholeExtent[idx*2]) 
      {
	displayExtent[idx*2] = wholeExtent[idx*2];
      }
    }


  int clip[2] = {0};
  this->GetActorClipSize(viewport, actor, &clip[0], &clip[1]);
  // Add the clipped size of the actor to the minimum extent
  for (idx = 0; idx < 2; idx++)
    {
      displayExtent[idx*2+1] = displayExtent[idx*2] + clip[idx] - 1;
    }

  // Clip the maximum extents
  for (idx = 0; idx < 3; idx++)
    {
    if (displayExtent[idx*2+1] > wholeExtent[idx*2+1]) 
      {
      displayExtent[idx*2+1] = wholeExtent[idx*2+1];
      }
    }

  this->Input->SetUpdateExtent(displayExtent);

  // Get the region from the input
  data = this->Input->UpdateAndReturnData();
  if ( !data)
    {
    vtkErrorMacro(<< "Render: Could not get data from input.");
    return;
    }

  this->DisplayExtent[0] = displayExtent[0];
  this->DisplayExtent[1] = displayExtent[1];
  this->DisplayExtent[2] = displayExtent[2];
  this->DisplayExtent[3] = displayExtent[3];
  this->DisplayExtent[4] = displayExtent[4];
  this->DisplayExtent[5] = displayExtent[5];

  this->RenderData(viewport, data, actor);
}

void vtkImageMapper::SetImageExtent2(int extent)
{
  this->DisplayExtent[4] = extent;
  this->DisplayExtent[5] = extent;
}

int vtkImageMapper::GetImageExtent2()
{
  return this->DisplayExtent[4];
}





