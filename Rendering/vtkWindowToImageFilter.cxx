/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkWindowToImageFilter.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkWindowToImageFilter* vtkWindowToImageFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWindowToImageFilter");
  if(ret)
    {
    return (vtkWindowToImageFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWindowToImageFilter;
}




//----------------------------------------------------------------------------
vtkWindowToImageFilter::vtkWindowToImageFilter()
{
  this->Input = NULL;
  this->Magnification = 1;
}

//----------------------------------------------------------------------------
vtkWindowToImageFilter::~vtkWindowToImageFilter()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::SetInput(vtkWindow *input)
{
  if (input != this->Input)
    {
    if (this->Input) {this->Input->UnRegister(this);}
    this->Input = input;
    if (this->Input) {this->Input->Register(this);}
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkWindowToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  
  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  os << indent << "Magnification: " << this->Magnification << "\n";
}


//----------------------------------------------------------------------------
// This method returns the largest region that can be generated.
void vtkWindowToImageFilter::ExecuteInformation()
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }
  vtkImageData *out = this->GetOutput();
  
  // set the extent
  out->SetWholeExtent(0, 
                      this->Input->GetSize()[0]*this->Magnification - 1,
                      0, 
                      this->Input->GetSize()[1]*this->Magnification - 1,
                      0, 0);
  
  // set the spacing
  out->SetSpacing(1.0, 1.0, 1.0);
  
  // set the origin.
  out->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components
  out->SetNumberOfScalarComponents(3);
  out->SetScalarType(VTK_UNSIGNED_CHAR);
}



//----------------------------------------------------------------------------
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkWindowToImageFilter::ExecuteData(vtkDataObject *vtkNotUsed(data))
{
  vtkImageData *out = this->GetOutput();
  out->SetExtent(out->GetWholeExtent());
  out->AllocateScalars();
  
  int outIncrY;
  int size[2];
  unsigned char *pixels, *outPtr, *pixels1;
  int idxY, rowSize;
  int i;
  
  if (out->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // get the size of the render window
  size[0] = this->Input->GetSize()[0];
  size[1] = this->Input->GetSize()[1];
  rowSize = size[0]*3;
  outIncrY = size[0]*this->Magnification*3;
    
  float *viewAngles;
  double *windowCenters;
  vtkRenderWindow *renWin = vtkRenderWindow::SafeDownCast(this->Input);
  if (!renWin)
    {
    vtkWarningMacro("The window passed to window to image should be a RenderWIndow or one of its subclasses");
    return;
    }
  
  vtkRendererCollection *rc = renWin->GetRenderers();
  vtkRenderer *aren;
  vtkCamera *cam;
  int numRenderers = rc->GetNumberOfItems();

  // for each renderer
  viewAngles = new float [numRenderers];
  windowCenters = new double [numRenderers*2];
  rc->InitTraversal();
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextItem();
    // store the old view angle & set the new
    cam = aren->GetActiveCamera();
    cam->GetWindowCenter(windowCenters+i*2);
    viewAngles[i] = cam->GetViewAngle();
    cam->SetViewAngle(asin(sin(viewAngles[i]*3.1415926/360.0)/
                           this->Magnification) 
                      * 360.0 / 3.1415926);
    cam->SetParallelScale(cam->GetParallelScale()/this->Magnification);
    }
  
  // render each of the tiles required to fill this request
  this->Input->SetTileScale(this->Magnification);
  this->Input->GetSize();
  
  int x, y;
  for (y = 0; y < this->Magnification; y++)
    {
    for (x = 0; x < this->Magnification; x++)
      {
      // setup the Window ivars
      this->Input->SetTileViewport((float)x/this->Magnification,
                                   (float)y/this->Magnification,
                                   (x+1.0)/this->Magnification,
                                   (y+1.0)/this->Magnification);
      
      // for each renderer, setup camera
      rc->InitTraversal();
      for (i = 0; i < numRenderers; ++i)
        {
        aren = rc->GetNextItem();
        cam = aren->GetActiveCamera();
        cam->SetWindowCenter(
          x*2 - this->Magnification*(1-windowCenters[i*2]) + 1, 
          y*2 - this->Magnification*(1-windowCenters[i*2+1]) + 1);
        }
      
      // now render the tile and get the data
      this->Input->Render();
      pixels = this->Input->GetPixelData(0,0,size[0] - 1,
                                         size[1] - 1, 1);
      pixels1 = pixels;
      
      // now for each renderer copy its pixels to the correct location
      rc->InitTraversal();
      for (i = 0; i < numRenderers; ++i)
        {
        aren = rc->GetNextItem();
        int pixelx, pixely;
        int finalx, finaly;
        
        float *vport;
        vport = aren->GetViewport();
        float *tileViewPort = this->Input->GetTileViewport();

        float vpu, vpv;
        pixelx = (int)(size[0]*vport[0]+0.5);
        pixely = (int)(size[1]*vport[1]+0.5);
        vpu = vport[0] + tileViewPort[0]*(vport[2] - vport[0]);
        vpv = vport[1] + tileViewPort[1]*(vport[3] - vport[1]);
        aren->NormalizedDisplayToDisplay(vpu,vpv);
        finalx = (int)(vpu+0.5);
        finaly = (int)(vpv+0.5);
        vpu = vport[0] + tileViewPort[2]*(vport[2] - vport[0]);
        vpv = vport[1] + tileViewPort[3]*(vport[3] - vport[1]);
        aren->NormalizedDisplayToDisplay(vpu,vpv);
        int sizex = (int)(vpu + 0.5) - finalx;
        int sizey = (int)(vpv + 0.5) - finaly;  
        
        // now write the data to the output image
        outPtr = 
          (unsigned char *)out->GetScalarPointer(finalx, finaly, 0);
        
        pixels1 = pixels + pixelx*3 + pixely*rowSize;
          
        // Loop through ouput pixels
        for (idxY = 0; idxY < sizey; idxY++)
          {
          memcpy(outPtr,pixels1,sizex*3);
          outPtr += outIncrY;
          pixels1 = pixels1 + rowSize;
          }
        }
      
      // free the memory
      delete [] pixels;
      }
    }
  
  
  
  // restore settings
  // for each renderer
  rc->InitTraversal();
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextItem();
    // store the old view angle & set the new
    cam = aren->GetActiveCamera();
    cam->SetWindowCenter(windowCenters[i*2],windowCenters[i*2+1]);
    cam->SetViewAngle(viewAngles[i]);
    cam->SetParallelScale(cam->GetParallelScale()*this->Magnification);
    }
  delete [] viewAngles;
  delete [] windowCenters;
  
  // render each of the tiles required to fill this request
  this->Input->SetTileScale(1);
  this->Input->SetTileViewport(0.0,0.0,1.0,1.0);
  this->Input->GetSize();
}



