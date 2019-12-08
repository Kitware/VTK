/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPMergePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkSMPMergePoints.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPMergePoints);

//------------------------------------------------------------------------------
vtkSMPMergePoints::vtkSMPMergePoints() = default;

//------------------------------------------------------------------------------
vtkSMPMergePoints::~vtkSMPMergePoints() = default;

//------------------------------------------------------------------------------
void vtkSMPMergePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::InitializeMerge()
{
  this->AtomicInsertionId = this->InsertionPointId;
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::Merge(vtkSMPMergePoints* locator, vtkIdType idx, vtkPointData* outPd,
  vtkPointData* ptData, vtkIdList* idList)
{
  if (!locator->HashTable[idx])
  {
    return;
  }

  vtkIdList *bucket, *oldIdToMerge;
  float* floatOldDataArray = nullptr;

  if (!(bucket = this->HashTable[idx]))
  {
    this->HashTable[idx] = bucket = vtkIdList::New();
    bucket->Allocate(this->NumberOfPointsPerBucket / 2, this->NumberOfPointsPerBucket / 3);
    oldIdToMerge = locator->HashTable[idx];
    oldIdToMerge->Register(this);
    if (this->Points->GetData()->GetDataType() == VTK_FLOAT)
    {
      floatOldDataArray = static_cast<vtkFloatArray*>(locator->Points->GetData())->GetPointer(0);
    }
  }
  else
  {
    oldIdToMerge = vtkIdList::New();

    vtkIdType nbOfIds = bucket->GetNumberOfIds();
    vtkIdType nbOfOldIds = locator->HashTable[idx]->GetNumberOfIds();
    oldIdToMerge->Allocate(nbOfOldIds);

    vtkDataArray* dataArray = this->Points->GetData();
    vtkDataArray* oldDataArray = locator->Points->GetData();
    vtkIdType* idArray = bucket->GetPointer(0);
    vtkIdType* idOldArray = locator->HashTable[idx]->GetPointer(0);

    bool found;

    if (dataArray->GetDataType() == VTK_FLOAT)
    {
      float* floatDataArray = static_cast<vtkFloatArray*>(dataArray)->GetPointer(0);
      floatOldDataArray = static_cast<vtkFloatArray*>(oldDataArray)->GetPointer(0);

      for (vtkIdType oldIdIdx = 0; oldIdIdx < nbOfOldIds; ++oldIdIdx)
      {
        found = false;
        vtkIdType oldId = idOldArray[oldIdIdx];
        float* x = floatOldDataArray + 3 * oldId;
        float* pt;
        for (vtkIdType i = 0; i < nbOfIds; ++i)
        {
          vtkIdType existingId = idArray[i];
          pt = floatDataArray + 3 * existingId;
          if (x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2])
          {
            // point is already in the list, return 0 and set the id parameter
            found = true;
            idList->SetId(oldId, existingId);
            break;
          }
        }
        if (!found)
        {
          oldIdToMerge->InsertNextId(oldId);
        }
      }
    }
    else
    {
      for (vtkIdType oldIdIdx = 0; oldIdIdx < nbOfOldIds; ++oldIdIdx)
      {
        found = false;
        vtkIdType oldId = idOldArray[oldIdIdx];
        double* x = oldDataArray->GetTuple(oldId);
        double* pt;
        for (vtkIdType i = 0; i < nbOfIds; ++i)
        {
          vtkIdType existingId = idArray[i];
          pt = dataArray->GetTuple(existingId);
          if (x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2])
          {
            // point is already in the list, return 0 and set the id parameter
            found = true;
            idList->SetId(oldId, existingId);
            break;
          }
        }
        if (!found)
        {
          oldIdToMerge->InsertNextId(oldId);
        }
      }
    }
  }

  // points have to be added
  vtkIdType numberOfInsertions = oldIdToMerge->GetNumberOfIds();
  vtkIdType firstId = this->AtomicInsertionId;
  this->AtomicInsertionId += numberOfInsertions;
  bucket->Resize(bucket->GetNumberOfIds() + numberOfInsertions);
  for (vtkIdType i = 0; i < numberOfInsertions; ++i)
  {
    vtkIdType newId = firstId + i, oldId = oldIdToMerge->GetId(i);
    idList->SetId(oldId, newId);
    bucket->InsertNextId(newId);
    if (floatOldDataArray)
    {
      const float* pt = floatOldDataArray + 3 * oldId;
      this->Points->SetPoint(newId, pt);
    }
    else
    {
      this->Points->SetPoint(newId, locator->Points->GetPoint(oldId));
    }
    outPd->SetTuple(newId, oldId, ptData);
  }
  oldIdToMerge->UnRegister(this);
}

//------------------------------------------------------------------------------
void vtkSMPMergePoints::FixSizeOfPointArray()
{
  this->Points->SetNumberOfPoints(this->AtomicInsertionId);
}
