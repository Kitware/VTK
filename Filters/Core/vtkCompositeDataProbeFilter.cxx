/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataProbeFilter.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkCompositeDataProbeFilter);
//----------------------------------------------------------------------------
vtkCompositeDataProbeFilter::vtkCompositeDataProbeFilter()
{
  this->PassPartialArrays = false;
}

//----------------------------------------------------------------------------
vtkCompositeDataProbeFilter::~vtkCompositeDataProbeFilter()
{
}

//----------------------------------------------------------------------------
int vtkCompositeDataProbeFilter::FillInputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 1)
  {
    // We have to save vtkDataObject since this filter can work on vtkDataSet
    // and vtkCompositeDataSet consisting of vtkDataSet leaf nodes.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkCompositeDataProbeFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkCompositeDataProbeFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet *sourceDS = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkCompositeDataSet* sourceComposite = vtkCompositeDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    return 0;
  }

  if (!sourceDS && !sourceComposite)
  {
    vtkErrorMacro("vtkDataSet or vtkCompositeDataSet is expected as the input "
      "on port 1");
    return 0;
  }

  if (sourceDS)
  {
    // Superclass knowns exactly what to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->BuildFieldList(sourceComposite))
  {
    this->InitializeForProbing(input, output);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(sourceComposite->NewIterator());
    // We do reverse traversal, so that for hierarchical datasets, we traverse the
    // higher resolution blocks first.
    int idx=0;
    for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!sourceDS)
      {
        vtkErrorMacro("All leaves in the multiblock dataset must be vtkDataSet.");
        return 0;
      }

      if (sourceDS->GetNumberOfPoints() == 0)
      {
        continue;
      }

      this->DoProbing(input, idx, sourceDS, output);
      idx++;
    }
  }

  this->PassAttributeData(input, sourceComposite, output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataProbeFilter::InitializeOutputArrays(vtkPointData *outPD,
                                                         vtkIdType numPts)
{
  if (!this->PassPartialArrays)
  {
    this->Superclass::InitializeOutputArrays(outPD, numPts);
  }
  else
  {
    for (int cc=0; cc < outPD->GetNumberOfArrays(); cc++)
    {
      vtkDataArray* da = outPD->GetArray(cc);
      if (da)
      {
        da->SetNumberOfTuples(numPts);
        double null_value = 0.0;
        if (da->IsA("vtkDoubleArray") || da->IsA("vtkFloatArray"))
        {
          null_value = vtkMath::Nan();
        }
        da->Fill(null_value);
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkCompositeDataProbeFilter::BuildFieldList(vtkCompositeDataSet* source)
{
  delete this->PointList;
  delete this->CellList;
  this->PointList = 0;
  this->CellList = 0;

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(source->NewIterator());

  int numDatasets = 0;
  for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
  {
    vtkDataSet* sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!sourceDS)
    {
      vtkErrorMacro("All leaves in the multiblock dataset must be vtkDataSet.");
      return 0;
    }
    if (sourceDS->GetNumberOfPoints() == 0)
    {
      continue;
    }
    numDatasets++;
  }

  this->PointList = new vtkDataSetAttributes::FieldList(numDatasets);
  this->CellList = new vtkDataSetAttributes::FieldList(numDatasets);

  bool initializedPD = false;
  bool initializedCD = false;
  for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
  {
    vtkDataSet* sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (sourceDS->GetNumberOfPoints() == 0)
    {
      continue;
    }
    if (!initializedPD)
    {
      this->PointList->InitializeFieldList(sourceDS->GetPointData());
      initializedPD = true;
    }
    else
    {
      if (this->PassPartialArrays)
      {
        this->PointList->UnionFieldList(sourceDS->GetPointData());
      }
      else
      {
        this->PointList->IntersectFieldList(sourceDS->GetPointData());
      }
    }

    if (sourceDS->GetNumberOfCells() > 0)
    {
      if (!initializedCD)
      {
        this->CellList->InitializeFieldList(sourceDS->GetCellData());
        initializedCD = true;
      }
      else
      {
        if (this->PassPartialArrays)
        {
          this->CellList->UnionFieldList(sourceDS->GetCellData());
        }
        else
        {
          this->CellList->IntersectFieldList(sourceDS->GetCellData());
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "PassPartialArrays: " << this->PassPartialArrays << endl;
}

