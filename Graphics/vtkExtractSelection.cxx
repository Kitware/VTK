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

vtkCxxRevisionMacro(vtkExtractSelection, "1.12");
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
    vtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
    int passThrough = 0;
    if (inInfo)
      {
      vtkSelection *sel = vtkSelection::SafeDownCast(
        selInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (sel->GetProperties()->Has(vtkSelection::PRESERVE_TOPOLOGY()) &&
          sel->GetProperties()->Get(vtkSelection::PRESERVE_TOPOLOGY()) != 0)
        {
        passThrough = 1;
        }
      }

    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
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
        this->GetOutputPortInformation(i)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
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
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the selection, input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! sel )
    {
    vtkErrorMacro(<<"No selection specified");
    return 1;
    }

  vtkDebugMacro(<< "Extracting from dataset");

  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()))
    {
    return 1;
    }
  
  int seltype = sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE());
  switch (seltype)
    {
    case vtkSelection::GLOBALIDS:
    case vtkSelection::VALUES:
    case vtkSelection::INDICES:
    {
    return this->ExtractIds(sel, input, output);
    }
    case vtkSelection::FRUSTUM:
    {
    return this->ExtractFrustum(sel, input, output);
    }
    case vtkSelection::LOCATIONS:
    {
    return this->ExtractLocations(sel, input, output);
    }
    case vtkSelection::THRESHOLDS:
    {
    return this->ExtractThresholds(sel, input, output);
    }
    default:
      return 1;
    }
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractIds(
  vtkSelection *sel, vtkDataSet* input, vtkDataSet *output)
{
  this->IdsFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->IdsFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->IdsFilter->Update();

  vtkDataSet* ecOutput = vtkDataSet::SafeDownCast(
    this->IdsFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  this->IdsFilter->SetInput(0, (vtkDataSet*)NULL);
  this->IdsFilter->SetInput(1, (vtkSelection*)NULL);  
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractFrustum(
  vtkSelection *sel, vtkDataSet* input, vtkDataSet *output)
{
  this->FrustumFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->FrustumFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->FrustumFilter->Update();

  vtkDataSet* ecOutput = vtkDataSet::SafeDownCast(
    this->FrustumFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  this->FrustumFilter->SetInput(0, (vtkDataSet*)NULL);
  this->FrustumFilter->SetInput(1, (vtkSelection*)NULL);

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractLocations(
  vtkSelection *sel, vtkDataSet* input, vtkDataSet *output)
{
  this->LocationsFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->LocationsFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->LocationsFilter->Update();

  vtkDataSet* ecOutput = vtkDataSet::SafeDownCast(
    this->LocationsFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  this->LocationsFilter->SetInput(0, (vtkDataSet*)NULL);
  this->LocationsFilter->SetInput(1, (vtkSelection*)NULL);

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractThresholds(
  vtkSelection *sel, vtkDataSet* input, vtkDataSet *output)
{
  this->ThresholdsFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->ThresholdsFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->ThresholdsFilter->Update();

  vtkDataSet* ecOutput = vtkDataSet::SafeDownCast(
    this->ThresholdsFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  this->ThresholdsFilter->SetInput(0, (vtkDataSet*)NULL);
  this->ThresholdsFilter->SetInput(1, (vtkSelection*)NULL);

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
    }
  return 1;
}
