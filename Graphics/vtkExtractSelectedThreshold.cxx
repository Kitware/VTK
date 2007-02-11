/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedThreshold.h"

#include "vtkDataSet.h"
#include "vtkThreshold.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"

vtkCxxRevisionMacro(vtkExtractSelectedThreshold, "1.1");
vtkStandardNewMacro(vtkExtractSelectedThreshold);

//----------------------------------------------------------------------------
vtkExtractSelectedThreshold::vtkExtractSelectedThreshold()
{
  this->SetNumberOfInputPorts(2);
  this->ThresholdFilter = vtkThreshold::New();
}

//----------------------------------------------------------------------------
vtkExtractSelectedThreshold::~vtkExtractSelectedThreshold()
{
  this->ThresholdFilter->Delete();
}

//----------------------------------------------------------------------------
int vtkExtractSelectedThreshold::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[1]->GetInformationObject(0);
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

  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::THRESHOLD)
    {
    return 1;
    }


  //make a shallow copy of my input for the internal filter
  vtkDataSet *ds = input->NewInstance();
  ds->ShallowCopy(input);
  this->ThresholdFilter->AddInputConnection(0, ds->GetProducerPort());

  //find out what we are suppose to threshold
  const char *array_name = NULL;
  if (sel->GetProperties()->Has(vtkSelection::NAME()))
    {
    array_name = sel->GetProperties()->Get(vtkSelection::NAME());
    }

  int field_type = vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS;
  if (sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()))
    {
    field_type = sel->GetProperties()->Get(vtkSelection::FIELD_TYPE());
    }
  switch (field_type)
    {
    case vtkSelection::POINT:
      field_type = vtkDataObject::FIELD_ASSOCIATION_POINTS;
      break;
    case vtkSelection::CELL:
    default:
      field_type = vtkDataObject::FIELD_ASSOCIATION_CELLS;      
      break;
    }  
  if (array_name != NULL)
    {
    this->ThresholdFilter->SetInputArrayToProcess(
      0,0,0,field_type,array_name);
    }
  else
    {
    this->ThresholdFilter->SetInputArrayToProcess(
      0,0,0,field_type, 
      vtkDataSetAttributes::SCALARS);
    }

  //find the values to threshold within
  //TODO: iterate over array to get a set of lower/upper limits
  vtkDoubleArray *lims = vtkDoubleArray::SafeDownCast(sel->GetSelectionList());
  double lower = lims->GetValue(0);
  double upper = lims->GetValue(1);
  this->ThresholdFilter->ThresholdBetween(lower, upper);

  //execute the threshold filter
  this->ThresholdFilter->Update();
  vtkDataSet* threshold_output = vtkDataSet::SafeDownCast(
    this->ThresholdFilter->GetOutputDataObject(0));
  output->ShallowCopy(threshold_output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
int vtkExtractSelectedThreshold::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  return 1;
}
