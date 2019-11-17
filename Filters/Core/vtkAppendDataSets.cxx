/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendDataSets.h"

#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetCollection.h"
#include "vtkExecutive.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkAppendDataSets);

//----------------------------------------------------------------------------
vtkAppendDataSets::vtkAppendDataSets()
  : MergePoints(false)
  , Tolerance(0.0)
  , ToleranceIsAbsolute(true)
  , OutputDataSetType(VTK_UNSTRUCTURED_GRID)
  , OutputPointsPrecision(DEFAULT_PRECISION)
{
}

//----------------------------------------------------------------------------
vtkAppendDataSets::~vtkAppendDataSets() {}

//----------------------------------------------------------------------------
vtkTypeBool vtkAppendDataSets::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Handle update requests
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkAppendDataSets::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  if (this->OutputDataSetType != VTK_POLY_DATA && this->OutputDataSetType != VTK_UNSTRUCTURED_GRID)
  {
    vtkErrorMacro(
      "Output type '" << vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
                      << "' is not supported.");
    return 0;
  }

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

    if (!output ||
      (vtkDataObjectTypes::GetTypeIdFromClassName(output->GetClassName()) !=
        this->OutputDataSetType))
    {
      vtkSmartPointer<vtkDataObject> newOutput;
      newOutput.TakeReference(vtkDataObjectTypes::NewDataObject(this->OutputDataSetType));
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendDataSets::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* outputUG =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* outputPD = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro("Appending data together");

  if (outputUG)
  {
    vtkNew<vtkAppendFilter> appender;
    appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    appender->SetMergePoints(this->GetMergePoints());
    appender->SetToleranceIsAbsolute(this->GetToleranceIsAbsolute());
    appender->SetTolerance(this->GetTolerance());

    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      auto input = vtkDataSet::GetData(inputVector[0], cc);
      appender->AddInputData(input);
    }
    if (appender->GetNumberOfInputConnections(0) > 0)
    {
      appender->Update();
      outputUG->ShallowCopy(appender->GetOutput());
    }
  }
  else if (outputPD)
  {
    vtkNew<vtkAppendPolyData> appender;
    appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      auto input = vtkPolyData::GetData(inputVector[0], cc);
      if (input)
      {
        appender->AddInputData(input);
      }
    }
    if (this->MergePoints)
    {
      if (appender->GetNumberOfInputConnections(0) > 0)
      {
        vtkNew<vtkCleanPolyData> cleaner;
        cleaner->SetInputConnection(appender->GetOutputPort());
        cleaner->PointMergingOn();
        cleaner->ConvertLinesToPointsOff();
        cleaner->ConvertPolysToLinesOff();
        cleaner->ConvertStripsToPolysOff();
        if (this->GetToleranceIsAbsolute())
        {
          cleaner->SetAbsoluteTolerance(this->GetTolerance());
          cleaner->ToleranceIsAbsoluteOn();
        }
        else
        {
          cleaner->SetTolerance(this->GetTolerance());
          cleaner->ToleranceIsAbsoluteOff();
        }
        cleaner->Update();
        output->ShallowCopy(cleaner->GetOutput());
      }
    }
    else
    {
      if (appender->GetNumberOfInputConnections(0) > 0)
      {
        appender->Update();
        output->ShallowCopy(appender->GetOutput());
      }
    }
  }
  else
  {
    vtkErrorMacro("Unsupported output type.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendDataSets::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  int numInputConnections = this->GetNumberOfInputConnections(0);

  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (int idx = 1; idx < numInputConnections; ++idx)
  {
    vtkInformation* inputInfo = inputVector[0]->GetInformationObject(idx);
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
int vtkAppendDataSets::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergePoints:" << (this->MergePoints ? "On" : "Off") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent
     << "OutputDataSetType: " << vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
     << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}
