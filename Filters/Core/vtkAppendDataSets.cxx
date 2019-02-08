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
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCleanPolyData.h"
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
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkAppendDataSets);

//----------------------------------------------------------------------------
vtkAppendDataSets::vtkAppendDataSets()
  : MergePoints(false)
  , Tolerance(0.0)
  , OutputPointsPrecision(DEFAULT_PRECISION)
{
}

//----------------------------------------------------------------------------
vtkAppendDataSets::~vtkAppendDataSets()
{
}

namespace
{

bool AreAllInputsPolyData(vtkInformationVector* inputVector)
{
  if (!inputVector)
  {
    return true;
  }

  for (int idx = 0; idx < inputVector->GetNumberOfInformationObjects(); ++idx)
  {
    if (!vtkPolyData::GetData(inputVector, idx))
    {
      return false;
    }
  }
  return true;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int vtkAppendDataSets::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  bool allPolyData = AreAllInputsPolyData(inputVector[0]);

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

    if (!output ||
      (output->IsA("vtkPolyData") && !allPolyData) ||
      (output->IsA("vtkUnstructuredGrid") && allPolyData))
    {
      vtkDataObject* newOutput = nullptr;
      // If all the inputs are vtkPolyData, produce a vtkPolyData to avoid
      // "demotion" to a vtkUnstructuredGrid.
      if (allPolyData)
      {
        newOutput = vtkPolyData::New();
      }
      else
      {
        newOutput = vtkUnstructuredGrid::New();
      }

      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendDataSets::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the output info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPointSet* output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* outputUG = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* outputPD = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro("Appending data together");

  if (outputUG)
  {
    vtkNew<vtkAppendFilter> appender;
    appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    appender->SetMergePoints(this->GetMergePoints());
    appender->SetTolerance(this->GetTolerance());
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      auto input = vtkDataSet::GetData(inputVector[0], cc);
      appender->AddInputData(input);
    }
    appender->Update();

    outputUG->ShallowCopy(appender->GetOutput());
  }
  else if (outputPD)
  {
    vtkNew<vtkAppendPolyData> appender;
    appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      auto input = vtkPolyData::GetData(inputVector[0], cc);
      appender->AddInputData(input);
    }
    if (this->MergePoints)
    {
      vtkNew<vtkCleanPolyData> cleaner;
      cleaner->SetInputConnection(appender->GetOutputPort());
      cleaner->PointMergingOn();
      cleaner->ConvertLinesToPointsOff();
      cleaner->ConvertPolysToLinesOff();
      cleaner->ConvertStripsToPolysOff();
      cleaner->SetTolerance(this->GetTolerance());
      cleaner->ToleranceIsAbsoluteOn();
      cleaner->Update();
      output->ShallowCopy(cleaner->GetOutput());
    }
    else
    {
      appender->Update();
      output->ShallowCopy(appender->GetOutput());
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
int vtkAppendDataSets::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MergePoints:" << (this->MergePoints?"On":"Off") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "OutputPointsPrecision: "
     << this->OutputPointsPrecision << "\n";
}
