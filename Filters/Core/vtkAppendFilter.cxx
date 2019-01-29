/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendFilter.h"

#include "vtkAppendDataSets.h"
#include "vtkDataSetCollection.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkAppendFilter);

//----------------------------------------------------------------------------
vtkAppendFilter::vtkAppendFilter()
{
  this->InputList = nullptr;
  this->MergePoints = 0;
  this->Tolerance = 0.0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
vtkAppendFilter::~vtkAppendFilter()
{
  if (this->InputList != nullptr)
  {
    this->InputList->Delete();
    this->InputList = nullptr;
  }
}

//----------------------------------------------------------------------------
vtkDataSet *vtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
  {
    return nullptr;
  }

  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInputData(vtkDataSet *ds)
{
  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for(int i=0; i<numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0,
        this->GetInputConnection(0, i));
    }
  }
}

//----------------------------------------------------------------------------
vtkDataSetCollection *vtkAppendFilter::GetInputList()
{
  if (this->InputList)
  {
    this->InputList->Delete();
  }
  this->InputList = vtkDataSetCollection::New();

  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    if (this->GetInput(idx))
    {
      this->InputList->AddItem(this->GetInput(idx));
    }
  }

  return this->InputList;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkNew<vtkAppendDataSets> appender;
  appender->SetMergePoints(this->GetMergePoints() ? 1 : 0);
  appender->SetTolerance(this->GetTolerance());
  appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
  appender->SetForceUnstructuredGridOutput(true);
  for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
  {
    vtkDataSet* dataset = vtkDataSet::GetData(inputVector[0], cc);
    appender->AddInputData(dataset);
  }

  // Set up progress observer to forward progress events to this class
  vtkNew<vtkEventForwarderCommand> progressForwarder;
  progressForwarder->SetTarget(this);
  appender->AddObserver(vtkCommand::ProgressEvent, progressForwarder);

  // Update the filter that does all the work and shallow copy the output
  appender->Update();
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);
  output->ShallowCopy(appender->GetOutput());

  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendFilter::RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *vtkNotUsed(outputVector))
{
  int numInputConnections = this->GetNumberOfInputConnections(0);

  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (int idx = 1; idx < numInputConnections; ++idx)
  {
    vtkInformation * inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MergePoints:" << (this->MergePoints?"On":"Off") << "\n";
  os << indent << "OutputPointsPrecision: "
     << this->OutputPointsPrecision << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
