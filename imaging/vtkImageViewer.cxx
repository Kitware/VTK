/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


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
#include "vtkImageViewer.h"
#ifdef _WIN32
#include "vtkImageWin32Viewer.h"
#else
#include "vtkImageXViewer.h"
#endif

//----------------------------------------------------------------------------
vtkImageViewer::vtkImageViewer()
{
  // Actual displayed extent is clipped.
  this->DisplayExtent[0] = -VTK_LARGE_INTEGER;
  this->DisplayExtent[1] = VTK_LARGE_INTEGER;
  this->DisplayExtent[2] = -VTK_LARGE_INTEGER;
  this->DisplayExtent[3] = VTK_LARGE_INTEGER;
  this->DisplayExtent[4] = -VTK_LARGE_INTEGER;
  this->DisplayExtent[5] = VTK_LARGE_INTEGER;
  
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->Position[0] = this->Position[1] = 0;
  this->Input = NULL;
  this->Mapped = 0;
  this->OwnWindow = 0;
  this->WindowName = NULL;
  
  this->ColorWindow = 255.0;
  this->ColorLevel = 127.0;
  this->GrayScaleHint = 0;
}


//----------------------------------------------------------------------------
vtkImageViewer::~vtkImageViewer()
{
}

//----------------------------------------------------------------------------
vtkImageViewer *vtkImageViewer::New()
{
#ifdef _WIN32
  return vtkImageWin32Viewer::New();
#else
  return vtkImageXViewer::New();
#endif  
}


//----------------------------------------------------------------------------
void vtkImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent << "DisplayExtent: (" << this->DisplayExtent[0] << ", " 
     << this->DisplayExtent[1] << ", " << this->DisplayExtent[2] << ", " 
     << this->DisplayExtent[3] << ", " << this->DisplayExtent[4] << ", " 
     << this->DisplayExtent[5] << ")\n";
  os << indent << "ColorWindow: " << this->ColorWindow << "\n";
  os << indent << "ColorLevel: " << this->ColorLevel << "\n";
  os << indent << "Size: " << this->Size[0] << ", " << this->Size[1] << "\n";
  os << indent << "Position: " << this->Position[0] << ", " 
     << this->Position[1] << "\n";
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
void vtkImageViewer::SetPosition(int x, int y)
{
  // if we arent mappen then just set the ivars 
  if (!this->Mapped)
    {
    if ((this->Position[0] != x)||(this->Position[1] != y))
      {
      this->Modified();
      }
    this->Position[0] = x;
    this->Position[1] = y;
    }
}



//----------------------------------------------------------------------------
void vtkImageViewer::Render(void)
{
  int idx;
  vtkImageData *data;
  int *wholeExtent;
  int displayExtent[6];
  
  if ( ! this->Input)
    {
    // open the window anyhow if not yet open
    this->RenderData(NULL);
    vtkDebugMacro(<< "Render: Please Set the input.");
    return;
    }

  this->Input->UpdateImageInformation();
  wholeExtent = this->Input->GetWholeExtent();
  
  // determine the Extent of the 2D input region needed
  // (intersection of DisplayExtent and WholeExtent).
  for (idx = 0; idx < 3; idx++)
    {
    if (this->DisplayExtent[idx*2] < wholeExtent[idx*2]) 
      {
      displayExtent[idx*2] = wholeExtent[idx*2];
      }
    else 
      {
      displayExtent[idx*2] = this->DisplayExtent[idx*2];
      }
    if (this->DisplayExtent[idx*2+1] > wholeExtent[idx*2+1]) 
      {
      displayExtent[idx*2+1] = wholeExtent[idx*2+1];
      }
    else 
      {
      displayExtent[idx*2+1] = this->DisplayExtent[idx*2+1];
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

  this->RenderData(data);
}




//----------------------------------------------------------------------------
int vtkImageViewer::GetZSlice()
{
  return this->DisplayExtent[4];
}

//----------------------------------------------------------------------------
void vtkImageViewer::SetZSlice(int val)
{
  this->DisplayExtent[4] = val;
  this->DisplayExtent[5] = val;
}


//----------------------------------------------------------------------------
int vtkImageViewer::GetWholeZMin()
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
int vtkImageViewer::GetWholeZMax()
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





