/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupProbeFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkMultiGroupProbeFilter);
vtkCxxRevisionMacro(vtkMultiGroupProbeFilter, "1.2");
//----------------------------------------------------------------------------
vtkMultiGroupProbeFilter::vtkMultiGroupProbeFilter()
{
}

//----------------------------------------------------------------------------
vtkMultiGroupProbeFilter::~vtkMultiGroupProbeFilter()
{
}

//----------------------------------------------------------------------------
int vtkMultiGroupProbeFilter::FillInputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 1)
    {
    // We have to save vtkDataObject since this filter can work on vtkDataSet
    // and vtkMultiGroupDataSet consisting of vtkDataSet leaf nodes.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkMultiGroupProbeFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkMultiGroupProbeFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet *sourceDS = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkMultiGroupDataSet* sourceMG = vtkMultiGroupDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input)
    {
    return 0;
    }

  double ibounds[6];
  input->GetBounds(ibounds);
  vtkBoundingBox ibb;
  ibb.SetBounds(ibounds);

  if (!sourceDS && !sourceMG)
    {
    vtkErrorMacro("vtkDataSet or vtkMultiGroupDataSet is expected as the input "
      "on port 1");
    return 0;
    }

  if (sourceDS)
    {
    // Superclass knowns exactly what to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  bool initialized = false;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(sourceMG->NewIterator());
  if (iter->IsA("vtkHierarchicalDataIterator"))
    {
    vtkHierarchicalDataIterator::SafeDownCast(iter)->SetAscendingLevels(0);
    }
  iter->VisitOnlyLeavesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!sourceDS)
      {
      vtkErrorMacro("All leaves in the multigroup dataset must be vtkDataSet.");
      return 0;
      }
    if (!initialized)
      {
      initialized = true;
      this->InitializeForProbing(input, sourceDS, output);
      }
    double sbounds[6];
    sourceDS->GetBounds(sbounds);
    vtkBoundingBox sbb;
    sbb.SetBounds(sbounds);
    // Probe only if the bounds of the two dataset intersect.
    if (sbb.Intersects(ibb))
      {
      this->ProbeEmptyPoints(input, sourceDS, output);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMultiGroupProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

