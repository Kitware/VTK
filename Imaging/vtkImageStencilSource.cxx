/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStencilSource.h"

#include "vtkImageStencilData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkImageStencilSource);
vtkCxxSetObjectMacro(vtkImageStencilSource, InformationInput, vtkImageData);

//----------------------------------------------------------------------------
vtkImageStencilSource::vtkImageStencilSource()
{
  this->InformationInput = NULL;

  this->OutputOrigin[0] = 0;
  this->OutputOrigin[1] = 0;
  this->OutputOrigin[2] = 0;

  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  this->OutputWholeExtent[0] = 0;
  this->OutputWholeExtent[1] = -1;
  this->OutputWholeExtent[2] = 0;
  this->OutputWholeExtent[3] = -1;
  this->OutputWholeExtent[4] = 0;
  this->OutputWholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
vtkImageStencilSource::~vtkImageStencilSource()
{
  this->SetInformationInput(NULL);
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InformationInput: " << this->InformationInput << "\n";

  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputWholeExtent: " << this->OutputWholeExtent[0] << " " <<
    this->OutputWholeExtent[1] << " " << this->OutputWholeExtent[2] << " " <<
    this->OutputWholeExtent[3] << " " << this->OutputWholeExtent[4] << " " <<
    this->OutputWholeExtent[5] << "\n";
}

//----------------------------------------------------------------------------
void vtkImageStencilSource::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->InformationInput,
                            "InformationInput");
}

//----------------------------------------------------------------------------
int vtkImageStencilSource::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  int wholeExtent[6];
  double spacing[3];
  double origin[3];

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  for (int i = 0; i < 3; i++)
    {
    wholeExtent[2*i] = this->OutputWholeExtent[2*i];
    wholeExtent[2*i+1] = this->OutputWholeExtent[2*i+1];
    spacing[i] = this->OutputSpacing[i];
    origin[i] = this->OutputOrigin[i];
    }

  // If InformationInput is set, then get the spacing,
  // origin, and whole extent from it.
  if (this->InformationInput)
    {
    this->InformationInput->GetExtent(wholeExtent);
    this->InformationInput->GetSpacing(spacing);
    this->InformationInput->GetOrigin(origin);
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::UNRESTRICTED_UPDATE_EXTENT(), 1);

  return 1;
}
