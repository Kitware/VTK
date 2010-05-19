/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSource.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"


//----------------------------------------------------------------------------
vtkImageSource::vtkImageSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);

  this->vtkSource::SetNthOutput(0,vtkImageData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkImageSource::SetOutput(vtkImageData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkImageSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkImageData *>(this->Outputs[0]);
}


//----------------------------------------------------------------------------
// Convert to Imaging API
void vtkImageSource::Execute()
{
  vtkImageData *output = this->GetOutput();

  // If we have multiple Outputs, they need to be allocate
  // in a subclass.  We cannot be sure all outputs are images.
  output->SetExtent(output->GetUpdateExtent());
  output->AllocateScalars();
  output->GetPointData()->GetScalars()->SetName("Scalars");
  
  this->Execute(this->GetOutput());
}

//----------------------------------------------------------------------------
// This function can be defined in a subclass to generate the data
// for a region.
void vtkImageSource::Execute(vtkImageData *)
{
  vtkErrorMacro(<< "Execute(): Method not defined.");
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageSource::AllocateOutputData(vtkDataObject *out)
{
  vtkImageData *res = vtkImageData::SafeDownCast(out);
  if (!res)
    {
    vtkWarningMacro("Call to AllocateOutputData with non vtkImageData output");
    return NULL;
    }

  // I would like to eliminate this method which requires extra "information"
  // That is not computed in the graphics pipeline.
  // Until I can eliminate the method, I will reexecute the ExecuteInformation
  // before the execute.
  this->ExecuteInformation();

  res->SetExtent(res->GetUpdateExtent());
  res->AllocateScalars();

  return res;
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageSource::GetOutput(int idx)
{
  return static_cast<vtkImageData *>(this->vtkSource::GetOutput(idx));
}

//----------------------------------------------------------------------------
int vtkImageSource::FillOutputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
