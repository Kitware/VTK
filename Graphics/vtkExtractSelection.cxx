/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelection.h"

#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkExtractSelectedIds.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkExtractSelectedLocations.h"
#include "vtkExtractSelectedThresholds.h"

#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkExtractSelection, "1.18");
vtkStandardNewMacro(vtkExtractSelection);

//----------------------------------------------------------------------------
vtkExtractSelection::vtkExtractSelection()
{
  this->SetNumberOfInputPorts(2);
  this->IdsFilter = vtkExtractSelectedIds::New();
  this->FrustumFilter = vtkExtractSelectedFrustum::New();
  this->LocationsFilter = vtkExtractSelectedLocations::New();
  this->ThresholdsFilter = vtkExtractSelectedThresholds::New();
}

//----------------------------------------------------------------------------
vtkExtractSelection::~vtkExtractSelection()
{
  this->IdsFilter->Delete();
  this->FrustumFilter->Delete();
  this->LocationsFilter->Delete();
  this->ThresholdsFilter->Delete();
}

//----------------------------------------------------------------------------
//needed because parent class sets output type to input type
//and we sometimes want to change it to make an UnstructuredGrid regardless of
//input type
int vtkExtractSelection::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input)
    {
    int passThrough = 0;
    vtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
    if (selInfo)
      {
      vtkSelection *sel = vtkSelection::SafeDownCast(
        selInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (sel->GetProperties()->Has(vtkSelection::PRESERVE_TOPOLOGY()) &&
          sel->GetProperties()->Get(vtkSelection::PRESERVE_TOPOLOGY()) != 0)
        {
        passThrough = 1;
        }
      }

    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataSet *output = vtkDataSet::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));

    if (!output
        ||
        (passThrough && !output->IsA(input->GetClassName()))
        ||
        (!passThrough && !output->IsA("vtkUnstructuredGrid"))
      )
      {
      vtkDataSet* newOutput = NULL;
      if (!passThrough)
        {
        // The mesh will be modified. 
        newOutput = vtkUnstructuredGrid::New();
        }
      else
        {
        // The mesh will not be modified.
        newOutput = input->NewInstance();
        }
      newOutput->SetPipelineInformation(info);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // verify the input, selection and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! input )
    {
    vtkErrorMacro(<<"No input specified");
    return 0;
    }

  if ( ! selInfo )
    {
    //When not given a selection, quietly select nothing.
    return 1;
    }
  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()))
    {
    vtkErrorMacro("Selection missing CONTENT_TYPE.");
    return 0;
    }
  
  vtkDataSetAlgorithm *subFilter = NULL;
  int seltype = sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE());
  switch (seltype)
    {
    case vtkSelection::GLOBALIDS:
    case vtkSelection::PEDIGREEIDS:
    case vtkSelection::VALUES:
    case vtkSelection::INDICES:
    {
    subFilter = this->IdsFilter;
    break;
    }
    case vtkSelection::FRUSTUM:
    {
    subFilter = this->FrustumFilter;
    break;
    }
    case vtkSelection::LOCATIONS:
    {
    subFilter = this->LocationsFilter;
    break;
    }
    case vtkSelection::THRESHOLDS:
    {
    subFilter = this->ThresholdsFilter;
    break;
    }
    default:
      vtkErrorMacro("Unrecognized CONTENT_TYPE.");
      return 0;
    }
  subFilter->SetInput(1, sel);

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      subFilter->GetExecutive());

  vtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  //pass all required information to the helper filter
  int piece = -1;
  int npieces = -1;
  int *uExtent;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    if (sddp)
      {
      sddp->SetUpdateExtent(0, piece, npieces, 0);
      }
    }
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    uExtent = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (sddp)
      {
      sddp->SetUpdateExtent(0, uExtent);
      }
    }
  
  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  subFilter->SetInput(0, inputCopy);

  subFilter->Update();

  vtkDataSet* ecOutput = vtkDataSet::SafeDownCast(
    subFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);

  //make sure everything is deallocated
  inputCopy->Delete();
  ecOutput->Initialize();
  subFilter->SetInput(0, (vtkDataSet*)NULL);
  subFilter->SetInput(1, (vtkSelection*)NULL);  
  return 1;
}


//----------------------------------------------------------------------------
void vtkExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkExtractSelection::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}
