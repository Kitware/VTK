/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageExport.h"

#include "vtkExecutive.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"

#include <ctype.h>
#include <string.h>

vtkStandardNewMacro(vtkImageExport);

//----------------------------------------------------------------------------
vtkImageExport::vtkImageExport()
{
  this->ImageLowerLeft = 1;
  this->ExportVoidPointer = 0;
  this->DataDimensions[0] = this->DataDimensions[1] =
    this->DataDimensions[2] = 0;
  this->LastPipelineMTime = 0;

  this->SetNumberOfOutputPorts(0);
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

vtkImageData *vtkImageExport::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
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
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkImageExport::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  // we are the end of the pipeline, we do nothing
  return 1;
}

//----------------------------------------------------------------------------
// Exports all the data from the input.
void vtkImageExport::Export(void *output)
{
  if (!this->GetPointerToData())
    {
    // GetPointerToData() outputs an error message.
    return;
    }
  
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
      ptr = static_cast<void *>(static_cast<char *>(ptr) + ysize*xsize*csize);
      for (int j = 0; j < ysize; j++)
        {
        ptr = static_cast<void *>(static_cast<char *>(ptr) - xsize*csize);
        memcpy(output, ptr, xsize*csize);
        output = static_cast<void *>(
          static_cast<char *>(output) + xsize*csize);
        }
      ptr = static_cast<void *>(static_cast<char *>(ptr) + ysize*xsize*csize);
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

double* vtkImageExport::SpacingCallbackFunction(void* userData)
{
  return static_cast<vtkImageExport*>(userData)->
    SpacingCallback();
}

double* vtkImageExport::OriginCallbackFunction(void* userData)
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
  if (this->GetInput())
    {
    this->GetInput()->UpdateInformation();
    }
}

int vtkImageExport::PipelineModifiedCallback()
{
  if (!this->GetInput())
    {
    return 0;
    }
  
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
  static int defaultextent[6] = {0,0,0,0,0,0};
  if (!this->GetInput())
    {
    return defaultextent;
    }
  else
    {
    return this->GetInput()->GetWholeExtent();
    }
}

double* vtkImageExport::SpacingCallback()
{
  static double defaultspacing[6] = {0.0,0.0,0.0};
  if (!this->GetInput())
    {
    return defaultspacing;
    }
  else
    {
  return this->GetInput()->GetSpacing();
    }
}

double* vtkImageExport::OriginCallback()
{
  static double defaultorigin[3] = {0.0,0.0,0.0};
  if (!this->GetInput())
    {
    return defaultorigin;
    }
  else
    {
    return this->GetInput()->GetOrigin();
    }
}

const char* vtkImageExport::ScalarTypeCallback()
{
  if (!this->GetInput())
    {
    return "unsigned char";
    }
  
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
    case VTK_SIGNED_CHAR:
      { return "signed char"; }
    default:
      { return "<unsupported>"; }
    }
}
  
int vtkImageExport::NumberOfComponentsCallback()
{
  if (!this->GetInput())
    {
    return 1;
    }
  else
    {
    return this->GetInput()->GetNumberOfScalarComponents();
    }
}

void vtkImageExport::PropagateUpdateExtentCallback(int* extent)
{
  if (this->GetInput())
    {
    this->GetInput()->SetUpdateExtent(extent);
    }
}

void vtkImageExport::UpdateDataCallback()
{
  if (this->GetInput())
    {
    this->GetInput()->Update();
    }
}

int* vtkImageExport::DataExtentCallback()
{
  static int defaultextent[6] = {0,0,0,0,0,0};
  if (!this->GetInput())
    {
    return defaultextent;
    }
  else
    {
    return this->GetInput()->GetExtent();
    }
}

void* vtkImageExport::BufferPointerCallback()
{
  if (!this->GetInput())
    {
    return static_cast<void *>(NULL);
    }
  else
    {
    return this->GetInput()->GetScalarPointer();
    }
}

int vtkImageExport::GetDataNumberOfScalarComponents() 
{
  if (this->GetInput() == NULL) 
    { 
    return 1; 
    }
  this->GetInput()->UpdateInformation();
  return this->GetInput()->GetNumberOfScalarComponents(); 
}

int vtkImageExport::GetDataScalarType() 
{
  if (this->GetInput() == NULL) 
    { 
    return VTK_UNSIGNED_CHAR; 
    }
  this->GetInput()->UpdateInformation();
  return this->GetInput()->GetScalarType();
}

int *vtkImageExport::GetDataExtent() 
{
  static int defaultextent[6] = {0, 0, 0, 0, 0, 0};
  if (this->GetInput() == NULL) { return defaultextent; }
  this->GetInput()->UpdateInformation();
  return this->GetInput()->GetWholeExtent(); 
}

void vtkImageExport::GetDataExtent(int *ptr) 
{
  if (this->GetInput() == NULL) { 
  ptr[0] = ptr[1] = ptr[2] = ptr[3] = ptr[4] = ptr[5] = 0; return; }
  this->GetInput()->UpdateInformation();
  this->GetInput()->GetWholeExtent(ptr); 
}
 
double *vtkImageExport::GetDataSpacing() 
{ 
  static double defaultspacing[3] = {1, 1, 1}; 
  if (this->GetInput() == NULL) { return defaultspacing; }
  this->GetInput()->UpdateInformation();
  return this->GetInput()->GetSpacing(); 
}

void vtkImageExport::GetDataSpacing(double *ptr) 
{ 
  if (this->GetInput() == NULL) { ptr[0] = ptr[1] = ptr[2] = 0.0; return; }
  this->GetInput()->UpdateInformation();
  this->GetInput()->GetSpacing(ptr); 
}

double *vtkImageExport::GetDataOrigin() 
{ 
  static double defaultorigin[3] = {0, 0, 0};
  if (this->GetInput() == NULL) { return defaultorigin; }
  this->GetInput()->UpdateInformation();
  return this->GetInput()->GetOrigin(); 
}

void vtkImageExport::GetDataOrigin(double *ptr) 
{ 
  if (this->GetInput() == NULL) { ptr[0] = ptr[1] = ptr[2] = 0.0; return; }
  this->GetInput()->UpdateInformation();
  this->GetInput()->GetOrigin(ptr); 
}
