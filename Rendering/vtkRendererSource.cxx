/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRendererSource.h"

#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkRendererSource, "1.55");
vtkStandardNewMacro(vtkRendererSource);

vtkCxxSetObjectMacro(vtkRendererSource,Input,vtkRenderer);

vtkRendererSource::vtkRendererSource()
{
  this->Input = NULL;
  this->WholeWindow = 0;
  this->RenderFlag = 0;
  this->DepthValues = 0;
  this->DepthValuesInScalars = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}


vtkRendererSource::~vtkRendererSource()
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

#ifdef VTK_USE_EXECUTIVES
void vtkRendererSource::AlgorithmExecute(vtkInformation *, 
                                         vtkInformationVector *, 
                                         vtkInformationVector *outputVector)
#else
void vtkRendererSource::ExecuteData(vtkDataObject *outp)
#endif  
{
  int numOutPts;
  float x1,y1,x2,y2;
  unsigned char *pixels, *ptr;
  int dims[3];
  
#ifdef VTK_USE_EXECUTIVES
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkImageData *output = 
    vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  int uExtent[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent);
  output->SetExtent(uExtent);  
#else
  vtkImageData *output = vtkImageData::SafeDownCast(outp);
  output->SetExtent(output->GetUpdateExtent());
#endif  

  output->AllocateScalars();
  vtkUnsignedCharArray *outScalars = 
    vtkUnsignedCharArray::SafeDownCast(output->GetPointData()->GetScalars());

  if (this->Input == NULL)
    {
    return;
    }

  if (this->DepthValuesInScalars)
    {
    outScalars->SetName("RGBValues");
    }
  else
    {
    outScalars->SetName("RGBZValues");
    }
  vtkRenderWindow *renWin;
  
  vtkDebugMacro(<<"Converting points");

  if (this->Input == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }

  renWin = this->Input->GetRenderWindow();
  if (renWin == NULL)
    {
    return;
    }
  
  if (this->RenderFlag)
    {
    renWin->Render();
    }
  
  // calc the pixel range for the renderer
  x1 = this->Input->GetViewport()[0]*
    ((this->Input->GetRenderWindow())->GetSize()[0] - 1);
  y1 = this->Input->GetViewport()[1]*
    ((this->Input->GetRenderWindow())->GetSize()[1] - 1);
  x2 = this->Input->GetViewport()[2]*
    ((this->Input->GetRenderWindow())->GetSize()[0] - 1);
  y2 = this->Input->GetViewport()[3]*
    ((this->Input->GetRenderWindow())->GetSize()[1] - 1);

  if (this->WholeWindow)
    {
    x1 = 0;
    y1 = 0;
    x2 = (this->Input->GetRenderWindow())->GetSize()[0] - 1;
    y2 = (this->Input->GetRenderWindow())->GetSize()[1] - 1;
    }
  
  // Get origin, aspect ratio and dimensions from this->Input
  dims[0] = (int)(x2 - x1 + 1); dims[1] = (int)(y2 -y1 + 1); dims[2] = 1;
  output->SetDimensions(dims);

  // Allocate data.  Scalar type is FloatScalars.
  numOutPts = dims[0] * dims[1];

  pixels = (this->Input->GetRenderWindow())->GetPixelData((int)x1,(int)y1,
                                                          (int)x2,(int)y2,1);

  // allocate scalars
  int nb_comp = output->GetNumberOfScalarComponents();
  ptr = outScalars->WritePointer(0, numOutPts * nb_comp);

  // copy scalars over (if only RGB is requested, use the pixels directly)
  if (!this->DepthValuesInScalars)
    {
    memcpy(ptr, pixels, numOutPts * nb_comp);
    }
  
  // Lets get the ZBuffer also, if requested.
  if (this->DepthValues || this->DepthValuesInScalars)
    {
    float *zBuf = (this->Input->GetRenderWindow())->GetZbufferData(
      (int)x1,(int)y1, (int)x2,(int)y2);

    // If RGBZ is requested, intermix RGB with shift/scaled Z
    if (this->DepthValuesInScalars)
      {
      float *zptr = zBuf, *zptr_end = zBuf + numOutPts;
      float min = *zBuf, max = *zBuf;
      while (zptr < zptr_end)
        {
        if (min < *zptr) { min = *zptr; }
        if (max > *zptr) { max = *zptr; }
        zptr++;
        }
      float scale = 255.0 / (max - min);

      zptr = zBuf;
      unsigned char *ppixels = pixels;
      while (zptr < zptr_end)
        {
        *ptr++ = *ppixels++;
        *ptr++ = *ppixels++;
        *ptr++ = *ppixels++;
        *ptr++ = static_cast<unsigned char>((*zptr++ - min) * scale);
        }
      }

    // If Z is requested as independent array, create it
    if (this->DepthValues)
      {
      vtkFloatArray *zArray = vtkFloatArray::New();
      zArray->Allocate(numOutPts);
      zArray->SetNumberOfTuples(numOutPts);
      float *zPtr = zArray->WritePointer(0, numOutPts);
      memcpy(zPtr,zBuf,numOutPts*sizeof(float));
      zArray->SetName("ZBuffer");
      output->GetPointData()->AddArray(zArray);
      zArray->Delete();
      }

    delete [] zBuf;
    }

  delete [] pixels;
}

void vtkRendererSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RenderFlag: " << (this->RenderFlag ? "On\n" : "Off\n");

  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  os << indent << "Whole Window: " << (this->WholeWindow ? "On\n" : "Off\n");
  os << indent << "Depth Values: " << (this->DepthValues ? "On\n" : "Off\n");
  os << indent << "Depth Values In Scalars: " << (this->DepthValuesInScalars ? "On\n" : "Off\n");
}


unsigned long vtkRendererSource::GetMTime()
{
  vtkRenderer *ren = this->GetInput();
  unsigned long t1 = this->MTime.GetMTime();
  unsigned long t2;

  if (!ren)
    {
    return t1;
    }

  // Update information on the input and
  // compute information that is general to vtkDataObject.
  t2 = ren->GetMTime();
  if (t2 > t1)
    {
    t1 = t2;
    }
  vtkActorCollection *actors = ren->GetActors();
  vtkCollectionSimpleIterator ait;
  actors->InitTraversal(ait);
  vtkActor *actor;
  vtkMapper *mapper;
  vtkDataSet *data;
  while ( (actor = actors->GetNextActor(ait)) )
    {
    t2 = actor->GetMTime();
    if (t2 > t1)
      {
      t1 = t2;
      }
    mapper = actor->GetMapper();
    if (mapper)
      {
      t2 = mapper->GetMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      data = mapper->GetInput();
      if (data)
        {
        data->UpdateInformation();
        }
      t2 = data->GetMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      t2 = data->GetPipelineMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      }  
    }

  return t1;
}


//----------------------------------------------------------------------------
// Consider renderer for PiplineMTime
#ifndef VTK_USE_EXECUTIVES
void vtkRendererSource::UpdateInformation()
{
  unsigned long t1;
  vtkImageData *output = this->GetOutput();
  vtkRenderer *ren = this->GetInput();
  float x1,y1,x2,y2;
  
  if (output == NULL || ren == NULL || ren->GetRenderWindow() == NULL)
    {
    return;
    }
  
  // calc the pixel range for the renderer
  x1 = ren->GetViewport()[0] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
  y1 = ren->GetViewport()[1] * ((ren->GetRenderWindow())->GetSize()[1] - 1);
  x2 = ren->GetViewport()[2] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
  y2 = ren->GetViewport()[3] *((ren->GetRenderWindow())->GetSize()[1] - 1);
  if (this->WholeWindow)
    {
    x1 = 0;
    y1 = 0;
    x2 = (this->Input->GetRenderWindow())->GetSize()[0] - 1;
    y2 = (this->Input->GetRenderWindow())->GetSize()[1] - 1;
    }    
  output->SetWholeExtent(0, static_cast<int>(x2-x1), 
                         0, static_cast<int>(y2-y1), 0, 0);
  output->SetScalarType(VTK_UNSIGNED_CHAR);
  output->SetNumberOfScalarComponents(3 + (this->DepthValuesInScalars ? 1:0));
  
  // Update information on the input and
  // compute information that is general to vtkDataObject.
  t1 = this->GetMTime();
  output->SetPipelineMTime(t1);
  this->InformationTime.Modified();
}
#endif

int vtkRendererSource::ProcessUpstreamRequest(
  vtkInformation *request, 
  vtkInformationVector *, 
  vtkInformationVector *)
{
#ifdef VTK_USE_EXECUTIVES
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    // we have no inputs so there is nothing to do
    return 1;
    }
#else
  (void)request;
#endif
  return 0;
}

int vtkRendererSource::ProcessDownstreamRequest(
  vtkInformation *request, 
  vtkInformationVector *inputVector, 
  vtkInformationVector *outputVector)
{
#ifdef VTK_USE_EXECUTIVES

  // this is basically execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkDebugMacro("ProcessDownstreamRequest(REQUEST_INFORMATION) "
                  "calling ExecuteInformation.");

    // Ask the subclass to fill in the information for the outputs.
    this->InvokeEvent(vtkCommand::ExecuteInformationEvent, NULL);

    // the executive has already passed all passable values to the output
    // information, we just need to change any that should be different from
    // the input
    vtkInformation* info = outputVector->GetInformationObject(0);

    vtkRenderer *ren = this->GetInput();
    if (ren == NULL || ren->GetRenderWindow() == NULL)
      {
      vtkErrorMacro("The input renderer has not been set yet!!!");
      return 0;
      }

    // calc the pixel range for the renderer
    float x1,y1,x2,y2;
    x1 = ren->GetViewport()[0] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
    y1 = ren->GetViewport()[1] * ((ren->GetRenderWindow())->GetSize()[1] - 1);
    x2 = ren->GetViewport()[2] * ((ren->GetRenderWindow())->GetSize()[0] - 1);
    y2 = ren->GetViewport()[3] *((ren->GetRenderWindow())->GetSize()[1] - 1);
    if (this->WholeWindow)
      {
      x1 = 0;
      y1 = 0;
      x2 = (this->Input->GetRenderWindow())->GetSize()[0] - 1;
      y2 = (this->Input->GetRenderWindow())->GetSize()[1] - 1;
      }    
    int extent[6] = {0, static_cast<int>(x2-x1), 
                     0, static_cast<int>(y2-y1), 
                     0, 0};

    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
    info->Set(vtkDataObject::SCALAR_TYPE(), VTK_UNSIGNED_CHAR);
    info->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),
              3 + (this->DepthValuesInScalars ? 1:0));
    
    // make sure the output is there
    vtkImageData *output = 
      vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    
    if (!output)
      {
      output = vtkImageData::New();
      info->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      }
    output->SetScalarType(VTK_UNSIGNED_CHAR);
    output->SetNumberOfScalarComponents(
      3 + (this->DepthValuesInScalars ? 1:0));
    return 1;
    }

  
  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    // get the output data object
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkImageData *output = 
      vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    
    output->PrepareForNewData();
    
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    this->AbortExecute = 0;
    this->Progress = 0.0;

    this->AlgorithmExecute(request, inputVector, outputVector);

    if(!this->AbortExecute)
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    // Mark the data as up-to-date.
    output->DataHasBeenGenerated();
    return 1;
    }
  return 0;
#else
  return this->Superclass::ProcessDownstreamRequest(request, inputVector,
                                                    outputVector);
#endif
}

  
int vtkRendererSource::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // invoke super first
  int retVal = this->Superclass::FillOutputPortInformation(port, info);
  
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  info->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  
  return retVal;
}

