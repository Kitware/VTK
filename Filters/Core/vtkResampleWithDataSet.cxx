/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResampleWithDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResampleWithDataSet.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"


vtkObjectFactoryNewMacro(vtkResampleWithDataSet);

//-----------------------------------------------------------------------------
vtkResampleWithDataSet::vtkResampleWithDataSet()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkResampleWithDataSet::~vtkResampleWithDataSet()
{
}

//-----------------------------------------------------------------------------
void vtkResampleWithDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Prober->PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkResampleWithDataSet::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkResampleWithDataSet::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
void vtkResampleWithDataSet::SetPassCellArrays(bool arg)
{
  this->Prober->SetPassCellArrays(arg);
}

bool vtkResampleWithDataSet::GetPassCellArrays()
{
  // work arround for Visual Studio warning C4800:
  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
  return this->Prober->GetPassCellArrays() ? true : false;
}

void vtkResampleWithDataSet::SetPassPointArrays(bool arg)
{
  this->Prober->SetPassPointArrays(arg);
}

bool vtkResampleWithDataSet::GetPassPointArrays()
{
  return this->Prober->GetPassPointArrays() ? true : false;
}

void vtkResampleWithDataSet::SetPassFieldArrays(bool arg)
{
  this->Prober->SetPassFieldArrays(arg);
}

bool vtkResampleWithDataSet::GetPassFieldArrays()
{
  return this->Prober->GetPassFieldArrays() ? true : false;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkResampleWithDataSet::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Prober->GetMTime());
}

//-----------------------------------------------------------------------------
int vtkResampleWithDataSet::RequestUpdateExtent(vtkInformation *,
                                                vtkInformationVector **inputVector,
                                                vtkInformationVector *)
{
  // This filter always asks for whole extent downstream. To resample
  // a subset of a structured input, you need to use ExtractVOI.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
                6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkResampleWithDataSet::FillInputPortInformation(int vtkNotUsed(port),
                                                     vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkResampleWithDataSet::FillOutputPortInformation(int vtkNotUsed(port),
                                                      vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
const char* vtkResampleWithDataSet::GetMaskArrayName() const
{
  return this->Prober->GetValidPointMaskArrayName();
}

//-----------------------------------------------------------------------------
void vtkResampleWithDataSet::SetBlankPointsAndCells(vtkDataSet *dataset)
{
  if (dataset->GetNumberOfPoints() <= 0)
  {
    return;
  }

  vtkPointData *pd = dataset->GetPointData();
  vtkCharArray *maskArray = vtkArrayDownCast<vtkCharArray>(
    pd->GetArray(this->GetMaskArrayName()));
  char *mask = maskArray->GetPointer(0);

  dataset->AllocatePointGhostArray();
  vtkUnsignedCharArray *pointGhostArray = dataset->GetPointGhostArray();
  vtkIdType numPoints = dataset->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    if (!mask[i])
    {
      pointGhostArray->SetValue(i, pointGhostArray->GetValue(i) |
                                   vtkDataSetAttributes::HIDDENPOINT);
    }
  }

  dataset->AllocateCellGhostArray();
  vtkUnsignedCharArray *cellGhostArray = dataset->GetCellGhostArray();
  vtkNew<vtkIdList> cellPoints;
  vtkIdType numCells = dataset->GetNumberOfCells();
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    dataset->GetCellPoints(i, cellPoints.GetPointer());
    vtkIdType npts = cellPoints->GetNumberOfIds();
    for (vtkIdType j = 0; j < npts; ++j)
    {
      vtkIdType ptid = cellPoints->GetId(j);
      if (!mask[ptid])
      {
        cellGhostArray->SetValue(i, cellGhostArray->GetValue(i) |
                                    vtkDataSetAttributes::HIDDENPOINT);
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
int vtkResampleWithDataSet::RequestData(vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject *source = sourceInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkDataObject *inDataObject = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *outDataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (inDataObject->IsA("vtkDataSet"))
  {
    vtkDataSet *input = vtkDataSet::SafeDownCast(inDataObject);
    vtkDataSet *output = vtkDataSet::SafeDownCast(outDataObject);

    this->Prober->SetInputData(input);
    this->Prober->SetSourceData(source);
    this->Prober->Update();
    output->ShallowCopy(this->Prober->GetOutput());
    this->SetBlankPointsAndCells(output);
  }
  else if (inDataObject->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(inDataObject);
    vtkCompositeDataSet *output = vtkCompositeDataSet::SafeDownCast(outDataObject);
    output->CopyStructure(input);

    this->Prober->SetSourceData(source);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(input->NewIterator());
    for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet *ds = static_cast<vtkDataSet*>(iter->GetCurrentDataObject());
      if (ds)
      {
        this->Prober->SetInputData(ds);
        this->Prober->Update();
        vtkDataSet *result = this->Prober->GetOutput();

        vtkDataSet *block = result->NewInstance();
        block->DeepCopy(result);
        this->SetBlankPointsAndCells(block);
        output->SetDataSet(iter.GetPointer(), block);
        block->Delete();
      }
    }
  }

  return 1;
}
