/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPResampleFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPResampleFilter.h"

#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataObject.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPProbeFilter.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPResampleFilter);

vtkCxxSetObjectMacro(vtkPResampleFilter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPResampleFilter::vtkPResampleFilter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->UseInputBoundsOn();
  this->CustomSamplingBounds[0] = this->CustomSamplingBounds[2] = this->CustomSamplingBounds[4] = 0;
  this->CustomSamplingBounds[1] = this->CustomSamplingBounds[3] = this->CustomSamplingBounds[5] = 1;
  this->SamplingDimension[0] = this->SamplingDimension[1] = this->SamplingDimension[2] = 10;
}

//----------------------------------------------------------------------------
vtkPResampleFilter::~vtkPResampleFilter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkPResampleFilter::RequestInformation(vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  double *boundsToSample = (this->UseInputBounds == 1) ? input->GetBounds() : this->CustomSamplingBounds;

  int wholeExtend[6] = { 0, this->SamplingDimension[0]-1, 0, this->SamplingDimension[1]-1, 0, this->SamplingDimension[2]-1 };
  double spacing[3] = {
    (boundsToSample[1] - boundsToSample[0])/static_cast<double>(this->SamplingDimension[0]-1),
    (boundsToSample[3] - boundsToSample[2])/static_cast<double>(this->SamplingDimension[1]-1),
    (boundsToSample[5] - boundsToSample[4])/static_cast<double>(this->SamplingDimension[2]-1)
  };

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtend, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPResampleFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Create Image Data for resampling
  vtkNew<vtkImageData> source;
  double *boundsToSample = (this->UseInputBounds == 1) ? input->GetBounds() : this->CustomSamplingBounds;
  source->SetOrigin(boundsToSample[0], boundsToSample[2], boundsToSample[4]);
  source->SetDimensions(this->SamplingDimension);
  source->SetSpacing(
        (boundsToSample[1] - boundsToSample[0])/static_cast<double>(this->SamplingDimension[0]-1),
        (boundsToSample[3] - boundsToSample[2])/static_cast<double>(this->SamplingDimension[1]-1),
        (boundsToSample[5] - boundsToSample[4])/static_cast<double>(this->SamplingDimension[2]-1));

  // Probe data
  vtkNew<vtkPProbeFilter> probeFilter;
  probeFilter->SetController(this->Controller);
  probeFilter->SetSourceData(input);
  probeFilter->SetInputData(source.GetPointer());
  probeFilter->Update();
  output->ShallowCopy(probeFilter->GetOutput());

  return 1;
}

#include "vtkInformationIntegerVectorKey.h"
//----------------------------------------------------------------------------
int vtkPResampleFilter::RequestUpdateExtent(vtkInformation *,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkStreamingDemandDrivenPipeline::SetUpdateExtentToWholeExtent(inInfo);

  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkPResampleFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPResampleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller " << this->Controller << endl;
  os << indent << "UseInputBounds " << this->UseInputBounds << endl;
  if(this->UseInputBounds == 0)
    {
    os << indent << "CustomSamplingBounds ["
       << this->CustomSamplingBounds[0] << ", "
       << this->CustomSamplingBounds[1] << ", "
       << this->CustomSamplingBounds[2] << ", "
       << this->CustomSamplingBounds[3] << ", "
       << this->CustomSamplingBounds[4] << ", "
       << this->CustomSamplingBounds[5] << "]"
       << endl;
    }
  os << indent << "SamplingDimension "
     << this->SamplingDimension[0] << " x "
     << this->SamplingDimension[1] << " x "
     << this->SamplingDimension[2] << endl;
}
