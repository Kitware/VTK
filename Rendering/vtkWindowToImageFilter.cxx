/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindowToImageFilter.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkWindowToImageFilter, "1.37");
vtkStandardNewMacro(vtkWindowToImageFilter);

//----------------------------------------------------------------------------
vtkWindowToImageFilter::vtkWindowToImageFilter()
{
  this->Input = NULL;
  this->Magnification = 1;
  this->ReadFrontBuffer = 1;
  this->ShouldRerender = 1;
  this->Viewport[0] = 0;
  this->Viewport[1] = 0;
  this->Viewport[2] = 1;
  this->Viewport[3] = 1;
  this->InputBufferType = VTK_RGB;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
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
vtkImageData* vtkWindowToImageFilter::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
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
  this->Superclass::PrintSelf(os,indent);
  
  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  os << indent << "ReadFrontBuffer: " << this->ReadFrontBuffer << "\n";
  os << indent << "Magnification: " << this->Magnification << "\n";
  os << indent << "ShouldRerender: " << this->ShouldRerender << "\n";
  os << indent << "Viewport: " << this->Viewport[0] << "," << this->Viewport[1] 
     << "," << this->Viewport[2] << "," << this->Viewport[3] << "\n";
  os << indent << "InputBufferType: " << this->InputBufferType << "\n";
}


//----------------------------------------------------------------------------
// This method returns the largest region that can be generated.
void vtkWindowToImageFilter::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }
  
  if(this->Magnification > 1 &&
     (this->Viewport[0] != 0 || this->Viewport[1] != 0 || 
      this->Viewport[2] != 1 || this->Viewport[3] != 1))
    {
    vtkWarningMacro(<<"Viewport extents are not used when Magnification > 1");
    this->Viewport[0] = 0;
    this->Viewport[1] = 0;
    this->Viewport[2] = 1;
    this->Viewport[3] = 1; 
    }
  
 
  // set the extent
  int *size = this->Input->GetSize();
  int wExtent[6];
  wExtent[0]= 0;
  wExtent[1] = int((this->Viewport[2] - this->Viewport[0])*size[0] + 0.5)*
    this->Magnification - 1;
  wExtent[2] = 0;
  wExtent[3] = int((this->Viewport[3] - this->Viewport[1])*size[1] + 0.5)*
    this->Magnification - 1;
  wExtent[4] = 0;
  wExtent[5] = 0;

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent, 6);

  switch( this->InputBufferType )
    {
    case VTK_RGB:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);
      break;
    case VTK_RGBA:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 4);
      break;
    case VTK_ZBUFFER:
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
      break;
    default:
      // VTK_RGB configuration by default
      vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 3);
      break;
    }
}

//----------------------------------------------------------------------------
int vtkWindowToImageFilter::ProcessRequest(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->RequestData(request, inputVector, outputVector);
    return 1;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkWindowToImageFilter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *out = 
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  out->SetExtent(out->GetUpdateExtent());
  out->AllocateScalars();

  if (!this->Input)
    {
    return;
    }

  int outIncrY;
  int size[2],winsize[2];
  int idxY, rowSize;
  int i;
  
  if (!  ((out->GetScalarType() == VTK_UNSIGNED_CHAR &&
           (this->InputBufferType == VTK_RGB || this->InputBufferType == VTK_RGBA)) ||
          (out->GetScalarType() == VTK_FLOAT && this->InputBufferType == VTK_ZBUFFER)))
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // get the size of the render window
  winsize[0] = this->Input->GetSize()[0];
  winsize[1] = this->Input->GetSize()[1];

  size[0] = int(winsize[0]*(this->Viewport[2]-this->Viewport[0]) + 0.5);
  size[1] = int(winsize[1]*(this->Viewport[3]-this->Viewport[1]) + 0.5);
  
  rowSize = size[0]*out->GetNumberOfScalarComponents();
  outIncrY = size[0]*this->Magnification*out->GetNumberOfScalarComponents();
    
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
  vtkCollectionSimpleIterator rsit;
  rc->InitTraversal(rsit);
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextRenderer(rsit);
    cams[i] = aren->GetActiveCamera();
    cams[i]->Register(this);
    cams[i]->GetWindowCenter(windowCenters+i*2);
    viewAngles[i] = cams[i]->GetViewAngle();
    parallelScale[i] = cams[i]->GetParallelScale();
    cam = cams[i]->NewInstance();
    cam->SetPosition(cams[i]->GetPosition());
    cam->SetFocalPoint(cams[i]->GetFocalPoint());    
    cam->SetViewUp(cams[i]->GetViewUp());
    cam->SetClippingRange(cams[i]->GetClippingRange());
    cam->SetParallelProjection(cams[i]->GetParallelProjection());
    cam->SetFocalDisk(cams[i]->GetFocalDisk());
    cam->SetUserTransform(cams[i]->GetUserTransform());
    cam->SetViewShear(cams[i]->GetViewShear());
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
      this->Input->SetTileViewport((double)x/this->Magnification,
                                   (double)y/this->Magnification,
                                   (x+1.0)/this->Magnification,
                                   (y+1.0)/this->Magnification);
      double *tvp = this->Input->GetTileViewport();
      
      // for each renderer, setup camera
      rc->InitTraversal(rsit);
      for (i = 0; i < numRenderers; ++i)
        {
        aren = rc->GetNextRenderer(rsit);
        cam = aren->GetActiveCamera();
        double *vp = aren->GetViewport();
        double visVP[4];
        visVP[0] = (vp[0] < tvp[0]) ? tvp[0] : vp[0];
        visVP[0] = (visVP[0] > tvp[2]) ? tvp[2] : visVP[0];
        visVP[1] = (vp[1] < tvp[1]) ? tvp[1] : vp[1];
        visVP[1] = (visVP[1] > tvp[3]) ? tvp[3] : visVP[1];
        visVP[2] = (vp[2] > tvp[2]) ? tvp[2] : vp[2];
        visVP[2] = (visVP[2] < tvp[0]) ? tvp[0] : visVP[2];
        visVP[3] = (vp[3] > tvp[3]) ? tvp[3] : vp[3];
        visVP[3] = (visVP[3] < tvp[1]) ? tvp[1] : visVP[3];

        // compute magnification
        double mag = (visVP[3] - visVP[1])/(vp[3] - vp[1]);
        // compute the delta
        double deltax = (visVP[2] + visVP[0])/2.0 - (vp[2] + vp[0])/2.0;
        double deltay = (visVP[3] + visVP[1])/2.0 - (vp[3] + vp[1])/2.0;
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
      if (this->ShouldRerender || this->Magnification > 1)
        {
        this->Input->Render();
        }

      int buffer = this->ReadFrontBuffer;
      if(!this->Input->GetDoubleBuffer())
        {
        buffer = 1;
        }      
      if (this->InputBufferType == VTK_RGB || this->InputBufferType == VTK_RGBA)
        {
        unsigned char *pixels, *pixels1, *outPtr;
        if (this->InputBufferType == VTK_RGB)
          {
          pixels = this->Input->GetPixelData(int(this->Viewport[0]* winsize[0]),
                                             int(this->Viewport[1]* winsize[1]),
                                             int(this->Viewport[2]* winsize[0] + 0.5) - 1,  
                                             int(this->Viewport[3]* winsize[1] + 0.5) - 1, buffer);
          } 
        else 
          {
          pixels = renWin->GetRGBACharPixelData(int(this->Viewport[0]* winsize[0]),
                                                int(this->Viewport[1]* winsize[1]),
                                                int(this->Viewport[2]* winsize[0] + 0.5) - 1,  
                                                int(this->Viewport[3]* winsize[1] + 0.5) - 1, buffer);

          }

        pixels1 = pixels;

        // now write the data to the output image
        outPtr = 
          (unsigned char *)out->GetScalarPointer(x*size[0],y*size[1], 0);
        for (idxY = 0; idxY < size[1]; idxY++)
          {
          memcpy(outPtr,pixels1,rowSize);
          outPtr += outIncrY;
          pixels1 += rowSize;
          }
      
        // free the memory
        delete [] pixels;
        }
      else 
        { // VTK_ZBUFFER
        float *pixels, *pixels1, *outPtr;
        pixels = renWin->GetZbufferData(int(this->Viewport[0]* winsize[0]),
                                        int(this->Viewport[1]* winsize[1]),
                                        int(this->Viewport[2]* winsize[0] + 0.5) - 1,  
                                        int(this->Viewport[3]* winsize[1] + 0.5) - 1);

        pixels1 = pixels;

        // now write the data to the output image
        outPtr = 
          (float *)out->GetScalarPointer(x*size[0],y*size[1], 0);
        for (idxY = 0; idxY < size[1]; idxY++)
          {
          memcpy(outPtr,pixels1,rowSize*sizeof(float));
          outPtr += outIncrY;
          pixels1 += rowSize;
          }
      
        // free the memory
        delete [] pixels;
        }
      }
    }

  // restore settings
  // for each renderer
  rc->InitTraversal(rsit);
  for (i = 0; i < numRenderers; ++i)
    {
    aren = rc->GetNextRenderer(rsit);
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



int vtkWindowToImageFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
