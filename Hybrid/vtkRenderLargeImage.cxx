/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderLargeImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderLargeImage.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkRenderLargeImage, "1.29");
vtkStandardNewMacro(vtkRenderLargeImage);

vtkCxxSetObjectMacro(vtkRenderLargeImage,Input,vtkRenderer);

//----------------------------------------------------------------------------
vtkRenderLargeImage::vtkRenderLargeImage()
{
  this->Input = NULL;
  this->Magnification = 3;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkRenderLargeImage::~vtkRenderLargeImage()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkRenderLargeImage::PrintSelf(ostream& os, vtkIndent indent)
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

  os << indent << "Magnification: " << this->Magnification << "\n";
}


//----------------------------------------------------------------------------
vtkImageData* vtkRenderLargeImage::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkRenderLargeImage::ProcessRequest(vtkInformation* request,
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
// Description:
// This method returns the largest region that can be generated.
void vtkRenderLargeImage::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  // set the extent, if the VOI has not been set then default to
  int wExt[6];
  wExt[0] = 0; wExt[2] = 0; wExt[4] = 0; wExt[5] = 0;
  wExt[1] = this->Magnification*
    this->Input->GetRenderWindow()->GetSize()[0] - 1;
  wExt[3] = this->Magnification*
    this->Input->GetRenderWindow()->GetSize()[1] - 1;
  
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);

  // set the spacing
  outInfo->Set(vtkDataObject::SPACING(),1.0, 1.0, 1.0);

  // set the origin.
  outInfo->Set(vtkDataObject::ORIGIN(),0.0, 0.0, 0.0);
  
  // set the scalar components
  outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(), 3);
  outInfo->Set(vtkDataObject::SCALAR_TYPE(), VTK_UNSIGNED_CHAR);
}



//----------------------------------------------------------------------------
// Description:
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkRenderLargeImage::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *data = 
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  data->SetExtent(data->GetUpdateExtent());
  data->AllocateScalars();
  int inExtent[6];
  int inIncr[3];
  int *size;
  int inWindowExtent[4];
  double viewAngle, parallelScale, windowCenter[2];
  vtkCamera *cam;
  unsigned char *pixels, *outPtr;
  int x, y, row;
  int rowSize, rowStart, rowEnd, colStart, colEnd;
  int doublebuffer;
  int swapbuffers = 0;
  
  if (this->GetOutput()->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("mismatch in scalar types!");
    return;
    }
  
  // Get the requested extents.
  this->GetOutput()->GetUpdateExtent(inExtent);

  // get and transform the increments
  data->GetIncrements(inIncr);
  
  // get the size of the render window
  size = this->Input->GetRenderWindow()->GetSize();

  // convert the request into windows
  inWindowExtent[0] = inExtent[0]/size[0];
  inWindowExtent[1] = inExtent[1]/size[0];
  inWindowExtent[2] = inExtent[2]/size[1];
  inWindowExtent[3] = inExtent[3]/size[1];

  // store the old view angle & set the new
  cam = this->Input->GetActiveCamera();
  cam->GetWindowCenter(windowCenter);
  viewAngle = cam->GetViewAngle();
  parallelScale = cam->GetParallelScale();
  cam->SetViewAngle(asin(sin(viewAngle*3.1415926/360.0)/this->Magnification) 
                    * 360.0 / 3.1415926);
  cam->SetParallelScale(parallelScale/this->Magnification);
  
  // are we double buffering?  If so, read from back buffer ....
  doublebuffer = this->Input->GetRenderWindow()->GetDoubleBuffer();
  if (doublebuffer) 
    {
    // save swap buffer state to restore later
    swapbuffers = this->Input->GetRenderWindow()->GetSwapBuffers();
    this->Input->GetRenderWindow()->SetSwapBuffers(0);
    }

  // render each of the tiles required to fill this request
  for (y = inWindowExtent[2]; y <= inWindowExtent[3]; y++)
    {
    for (x = inWindowExtent[0]; x <= inWindowExtent[1]; x++)
      {
      cam->SetWindowCenter(x*2 - this->Magnification*(1-windowCenter[0]) + 1, 
                           y*2 - this->Magnification*(1-windowCenter[1]) + 1);
      this->Input->GetRenderWindow()->Render();
      pixels = this->Input->GetRenderWindow()->GetPixelData(0,0,size[0] - 1,
                                                            size[1] - 1,
                                                            !doublebuffer);

      // now stuff the pixels into the data row by row
      colStart = inExtent[0] - x*size[0];
      if (colStart < 0)
        {
        colStart = 0;
        }
      colEnd = size[0] - 1;
      if (colEnd > (inExtent[1] - x*size[0]))
        {
        colEnd = inExtent[1] - x*size[0];
        }
      rowSize = colEnd - colStart + 1;
          
      // get the output pointer and do arith on it if necc
      outPtr = 
        (unsigned char *)data->GetScalarPointer(inExtent[0],inExtent[2],0);
      outPtr = outPtr + (x*size[0] - inExtent[0])*inIncr[0] + 
        (y*size[1] - inExtent[2])*inIncr[1];

      rowStart = inExtent[2] - y*size[1];
      if (rowStart < 0)
        {
        rowStart = 0;
        }
      rowEnd = size[1] - 1;
      if (rowEnd > (inExtent[3] - y*size[1]))
        {
        rowEnd = (inExtent[3] - y*size[1]);
        }
      for (row = rowStart; row <= rowEnd; row++)
        {
        memcpy(outPtr + row*inIncr[1] + colStart*inIncr[0], 
               pixels + row*size[0]*3 + colStart*3, rowSize*3);
        }
      // free the memory
      delete [] pixels;
      }
    }

  // restore the state of the SwapBuffers bit before we mucked with it.
  if (doublebuffer && swapbuffers)
    {
    this->Input->GetRenderWindow()->SetSwapBuffers(swapbuffers);
    }

  cam->SetViewAngle(viewAngle);
  cam->SetParallelScale(parallelScale);
  cam->SetWindowCenter(windowCenter[0],windowCenter[1]);
}

int vtkRenderLargeImage::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
