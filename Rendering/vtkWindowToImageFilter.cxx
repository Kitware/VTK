/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.cxx
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkWindowToImageFilter.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkWindowToImageFilter, "1.10");
vtkStandardNewMacro(vtkWindowToImageFilter);

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
  unsigned char *pixels, *outPtr;
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
  vtkCamera **cams = new vtkCamera *[numRenderers];
  viewAngles = new float [numRenderers];
  windowCenters = new double [numRenderers*2];
  double *parallelScale = new double [numRenderers];
  rc->InitTraversal();
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextItem();
    cams[i] = aren->GetActiveCamera();
    cams[i]->Register(this);
    cams[i]->GetWindowCenter(windowCenters+i*2);
    viewAngles[i] = cams[i]->GetViewAngle();
    parallelScale[i] = cams[i]->GetParallelScale();
    cam = vtkCamera::New();
    cam->SetPosition(cams[i]->GetPosition());
    cam->SetFocalPoint(cams[i]->GetFocalPoint());    
    cam->SetViewUp(cams[i]->GetViewUp());
    cam->SetClippingRange(cams[i]->GetClippingRange());
    cam->SetParallelProjection(cams[i]->GetParallelProjection());
    cam->SetFocalDisk(cams[i]->GetFocalDisk());
    aren->SetActiveCamera(cam);
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
      float *tvp = this->Input->GetTileViewport();
      
      // for each renderer, setup camera
      rc->InitTraversal();
      for (i = 0; i < numRenderers; ++i)
        {
        aren = rc->GetNextItem();
        cam = aren->GetActiveCamera();
        float *vp = aren->GetViewport();
        float visVP[4];
        visVP[0] = (vp[0] >= tvp[0]) ? vp[0] : tvp[0];
        visVP[1] = (vp[1] >= tvp[1]) ? vp[1] : tvp[1];
        visVP[2] = (vp[2] <= tvp[2]) ? vp[2] : tvp[2];
        visVP[3] = (vp[3] <= tvp[3]) ? vp[3] : tvp[3];
        // compute magnification
        float mag = (visVP[3] - visVP[1])/(vp[3] - vp[1]);
        // compute the delta
        float deltax = (visVP[2] + visVP[0])/2.0 - (vp[2] + vp[0])/2.0;
        float deltay = (visVP[3] + visVP[1])/2.0 - (vp[3] + vp[1])/2.0;
        // scale by original window size
        if (visVP[2] - visVP[0] > 0)
          {
          deltax = 2.0*deltax/(visVP[2] - visVP[0]);
          }
        if (visVP[3] - visVP[1] > 0)
          {
          deltay = 2.0*deltay/(visVP[3] - visVP[1]);
          }
        cam->SetWindowCenter(deltax,deltay);        
        cam->SetViewAngle(asin(sin(viewAngles[i]*3.1415926/360.0)*mag) 
                          * 360.0 / 3.1415926);
        cam->SetParallelScale(parallelScale[i]*mag);
        }
      
      // now render the tile and get the data
      this->Input->Render();
      pixels = this->Input->GetPixelData(0,0,size[0] - 1,
                                         size[1] - 1, 1);
      unsigned char *pixels1 = pixels;
      
      // now write the data to the output image
      outPtr = 
        (unsigned char *)out->GetScalarPointer(x*size[0],y*size[1], 0);
      
      // Loop through ouput pixels
      for (idxY = 0; idxY < size[1]; idxY++)
        {
        memcpy(outPtr,pixels1,rowSize);
        outPtr += outIncrY;
        pixels1 += rowSize;
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
    aren->SetActiveCamera(cams[i]);
    cams[i]->UnRegister(this);
    cam->Delete();
    }
  delete [] viewAngles;
  delete [] windowCenters;
  delete [] parallelScale;
  delete [] cams;
  
  // render each of the tiles required to fill this request
  this->Input->SetTileScale(1);
  this->Input->SetTileViewport(0.0,0.0,1.0,1.0);
  this->Input->GetSize();
}



