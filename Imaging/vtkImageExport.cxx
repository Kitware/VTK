/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.cxx
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
#include "vtkImageExport.h"
#include "vtkObjectFactory.h"

#include <ctype.h>
#include <string.h>

vtkCxxRevisionMacro(vtkImageExport, "1.23");
vtkStandardNewMacro(vtkImageExport);

//----------------------------------------------------------------------------
vtkImageExport::vtkImageExport()
{
  this->ImageLowerLeft = 1;
  this->ExportVoidPointer = 0;
  this->DataDimensions[0] = this->DataDimensions[1] =
    this->DataDimensions[2] = 0;
  this->LastPipelineMTime = 0;
}

//----------------------------------------------------------------------------
vtkImageExport::~vtkImageExport()
{
}

//----------------------------------------------------------------------------
void vtkImageExport::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ImageLowerLeft: " 
     << (this->ImageLowerLeft ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkImageExport::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageExport::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int vtkImageExport::GetDataMemorySize()
{
  vtkImageData *input = this->GetInput();
  if (input == NULL)
    {
    return 0;
    }

  input->UpdateInformation();
  int *extent = input->GetWholeExtent();
  int size = input->GetScalarSize();
  size *= input->GetNumberOfScalarComponents();
  size *= (extent[1] - extent[0] + 1);
  size *= (extent[3] - extent[2] + 1);
  size *= (extent[5] - extent[4] + 1);

  return size;
}


//----------------------------------------------------------------------------
void vtkImageExport::GetDataDimensions(int *dims)
{
  vtkImageData *input = this->GetInput();
  if (input == NULL)
    {
    dims[0] = dims[1] = dims[2] = 0;
    return;
    }

  input->UpdateInformation();
  int *extent = input->GetWholeExtent();
  dims[0] = extent[1]-extent[0]+1;
  dims[1] = extent[3]-extent[2]+1;
  dims[2] = extent[5]-extent[4]+1;
}

//----------------------------------------------------------------------------
void vtkImageExport::SetExportVoidPointer(void *ptr)
{
  if (this->ExportVoidPointer == ptr)
    {
    return;
    }
  this->ExportVoidPointer = ptr;
  this->Modified();
}

//----------------------------------------------------------------------------
// Exports all the data from the input.
void vtkImageExport::Export(void *output)
{
  if (this->ImageLowerLeft)
    {
    memcpy(output,this->GetPointerToData(),this->GetDataMemorySize());
    }
  else
    { // flip the image when it is output
    void *ptr = this->GetPointerToData();
    int *extent = this->GetInput()->GetWholeExtent();
    int xsize = extent[1]-extent[0]+1;
    int ysize = extent[3]-extent[2]+1;
    int zsize = extent[5]-extent[4]+1;
    int csize = this->GetInput()->GetScalarSize()* \
                this->GetInput()->GetNumberOfScalarComponents();

    for (int i = 0; i < zsize; i++)
      {
      ptr = (void *)(((char *)ptr) + ysize*xsize*csize);
      for (int j = 0; j < ysize; j++)
        {
        ptr = (void *)(((char *)ptr) - xsize*csize);
        memcpy(output, ptr, xsize*csize);
        output = (void *)(((char *)output) + xsize*csize);
        }
      ptr = (void *)(((char *)ptr) + ysize*xsize*csize);
      }
    }
}

//----------------------------------------------------------------------------
// Provides a valid pointer to the data (only valid until the next
// update, though)

void *vtkImageExport::GetPointerToData()
{
  // Error checking
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Export: Please specify an input!");
    return 0;
    }

  vtkImageData *input = this->GetInput();
  input->UpdateInformation();
  input->SetUpdateExtent(input->GetWholeExtent());
  input->ReleaseDataFlagOff();

  input->Update();
  this->UpdateProgress(0.0);
  this->UpdateProgress(1.0);

  return input->GetScalarPointer();
}

//----------------------------------------------------------------------------
void* vtkImageExport::GetCallbackUserData()
{
  return this;
}

vtkImageExport::UpdateInformationCallbackType
vtkImageExport::GetUpdateInformationCallback() const
{
  return &vtkImageExport::UpdateInformationCallbackFunction;
}

vtkImageExport::PipelineModifiedCallbackType
vtkImageExport::GetPipelineModifiedCallback() const
{
  return &vtkImageExport::PipelineModifiedCallbackFunction;
}

vtkImageExport::WholeExtentCallbackType
vtkImageExport::GetWholeExtentCallback() const
{
  return &vtkImageExport::WholeExtentCallbackFunction;
}

vtkImageExport::SpacingCallbackType
vtkImageExport::GetSpacingCallback() const
{
  return &vtkImageExport::SpacingCallbackFunction;
}

vtkImageExport::OriginCallbackType
vtkImageExport::GetOriginCallback() const
{
  return &vtkImageExport::OriginCallbackFunction;
}

vtkImageExport::ScalarTypeCallbackType
vtkImageExport::GetScalarTypeCallback() const
{
  return &vtkImageExport::ScalarTypeCallbackFunction;
}

vtkImageExport::NumberOfComponentsCallbackType
vtkImageExport::GetNumberOfComponentsCallback() const
{
  return &vtkImageExport::NumberOfComponentsCallbackFunction;
}

vtkImageExport::PropagateUpdateExtentCallbackType
vtkImageExport::GetPropagateUpdateExtentCallback() const
{
  return &vtkImageExport::PropagateUpdateExtentCallbackFunction;
}

vtkImageExport::UpdateDataCallbackType
vtkImageExport::GetUpdateDataCallback() const
{
  return &vtkImageExport::UpdateDataCallbackFunction;
}

vtkImageExport::DataExtentCallbackType
vtkImageExport::GetDataExtentCallback() const
{
  return &vtkImageExport::DataExtentCallbackFunction;
}

vtkImageExport::BufferPointerCallbackType
vtkImageExport::GetBufferPointerCallback() const
{
  return &vtkImageExport::BufferPointerCallbackFunction;
}

//----------------------------------------------------------------------------
void vtkImageExport::UpdateInformationCallbackFunction(void* userData)
{
  static_cast<vtkImageExport*>(userData)->
    UpdateInformationCallback();
}

int vtkImageExport::PipelineModifiedCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    PipelineModifiedCallback();
}

int* vtkImageExport::WholeExtentCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    WholeExtentCallback();
}

float* vtkImageExport::SpacingCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    SpacingCallback();
}

float* vtkImageExport::OriginCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    OriginCallback();
}

const char* vtkImageExport::ScalarTypeCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    ScalarTypeCallback();
}
 
int vtkImageExport::NumberOfComponentsCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    NumberOfComponentsCallback();
}

void vtkImageExport::PropagateUpdateExtentCallbackFunction(void* userData,
                                                               int* extent)
{
  static_cast<vtkImageExport*>(userData)->
    PropagateUpdateExtentCallback(extent);
}

void vtkImageExport::UpdateDataCallbackFunction(void* userData)
{
  static_cast<vtkImageExport*>(userData)->
    UpdateDataCallback();
}

int* vtkImageExport::DataExtentCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    DataExtentCallback();
}

void* vtkImageExport::BufferPointerCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    BufferPointerCallback();
}


//----------------------------------------------------------------------------
void vtkImageExport::UpdateInformationCallback()
{
  this->GetInput()->UpdateInformation();
}

int vtkImageExport::PipelineModifiedCallback()
{
  unsigned long mtime = this->GetInput()->GetPipelineMTime();
  if(mtime > this->LastPipelineMTime)
    {
    this->LastPipelineMTime = mtime;
    return 1;
    }
  return 0;
}

int* vtkImageExport::WholeExtentCallback()
{
  return this->GetInput()->GetWholeExtent();
}

float* vtkImageExport::SpacingCallback()
{
  return this->GetInput()->GetSpacing();
}

float* vtkImageExport::OriginCallback()
{
  return this->GetInput()->GetOrigin();
}

const char* vtkImageExport::ScalarTypeCallback()
{
  switch (this->GetInput()->GetScalarType())
    {
    case VTK_DOUBLE:
      { return "double"; }
    case VTK_FLOAT:
      { return "float"; }
    case VTK_LONG:
      { return "long"; }
    case VTK_UNSIGNED_LONG:
      { return "unsigned long"; }
    case VTK_INT:
      { return "int"; }
    case VTK_UNSIGNED_INT:
      { return "unsigned int"; }
    case VTK_SHORT:
      { return "short"; }
    case VTK_UNSIGNED_SHORT:
      { return "unsigned short"; }
    case VTK_CHAR:
      { return "char"; }
    case VTK_UNSIGNED_CHAR:
      { return "unsigned char"; }
    default:
      { return "<unsupported>"; }
    }
}
  
int vtkImageExport::NumberOfComponentsCallback()
{
  return this->GetInput()->GetNumberOfScalarComponents();
}

void vtkImageExport::PropagateUpdateExtentCallback(int* extent)
{
  this->GetInput()->SetUpdateExtent(extent);
}

void vtkImageExport::UpdateDataCallback()
{
  this->GetInput()->Update();
}

int* vtkImageExport::DataExtentCallback()
{
  return this->GetInput()->GetExtent();
}

void* vtkImageExport::BufferPointerCallback()
{
  return this->GetInput()->GetScalarPointer();
}
