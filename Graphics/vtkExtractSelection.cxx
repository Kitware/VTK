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
#include "vtkUnstructuredGrid.h"
#include "vtkExtractSelectedIds.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkExtractSelectedLocations.h"
#include "vtkExtractSelectedThresholds.h"

vtkCxxRevisionMacro(vtkExtractSelection, "1.8");
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
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
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
    case vtkSelection::IDS:
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
  vtkSelection *sel, vtkDataSet* input, vtkUnstructuredGrid *output)
{
  this->IdsFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->IdsFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->IdsFilter->Update();

  vtkUnstructuredGrid* ecOutput = vtkUnstructuredGrid::SafeDownCast(
    this->IdsFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractFrustum(
  vtkSelection *sel, vtkDataSet* input, vtkUnstructuredGrid *output)
{
  this->FrustumFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->FrustumFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->FrustumFilter->Update();

  vtkUnstructuredGrid* ecOutput = vtkUnstructuredGrid::SafeDownCast(
    this->FrustumFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractLocations(
  vtkSelection *sel, vtkDataSet* input, vtkUnstructuredGrid *output)
{
  this->LocationsFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->LocationsFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->LocationsFilter->Update();

  vtkUnstructuredGrid* ecOutput = vtkUnstructuredGrid::SafeDownCast(
    this->LocationsFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::ExtractThresholds(
  vtkSelection *sel, vtkDataSet* input, vtkUnstructuredGrid *output)
{
  this->ThresholdsFilter->SetInput(1, sel);

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->ThresholdsFilter->SetInput(0, inputCopy);
  inputCopy->Delete();

  this->ThresholdsFilter->Update();

  vtkUnstructuredGrid* ecOutput = vtkUnstructuredGrid::SafeDownCast(
    this->ThresholdsFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

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
