// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUniformGridAMRIterator.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------
vtkStandardNewMacro(vtkUniformGridAMRIterator);

//----------------------------------------------------------------
vtkUniformGridAMRIterator::vtkUniformGridAMRIterator() = default;

//----------------------------------------------------------------
vtkUniformGridAMRIterator::~vtkUniformGridAMRIterator() = default;

//------------------------------------------------------------------------------
void vtkUniformGridAMRIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkUniformGridAMRIterator::GoToFirstItem()
{
  if (!this->IsValid())
  {
    return;
  }

  // Intialize the iterator
  this->InitializeInternal();
  this->CurrentLevel = 0;
  this->CurrentIndex = 0;

  // Go to the first item
  this->NextInternal();

  // This is important, as NextInternal increment the CurrentFlatIndex
  // and this iterator behave differently
  this->CurrentFlatIndex = 0;

  // Check if current item is valid
  this->CheckItemAndLoopIfNeeded();

  // When going to the first item, CurrentLevel would have counted the root,
  // which should not be, lets decrement it.
  this->CurrentLevel--;
}

//------------------------------------------------------------------------------
void vtkUniformGridAMRIterator::GoToNextItem()
{
  if (!this->IsValid())
  {
    return;
  }

  if (!this->IsDoneWithTraversal())
  {
    // Go to the next item
    this->CurrentIndex++;
    this->NextInternal();

    // Check if current item is valid
    this->CheckItemAndLoopIfNeeded();
  }
}

//----------------------------------------------------------------
unsigned int vtkUniformGridAMRIterator::GetCurrentLevel() const
{
  if (this->Reverse)
  {
    vtkErrorMacro("CurrentLevel cannot be obtained when iterating in reverse order.");
    return 0;
  }
  return this->CurrentLevel;
}

//----------------------------------------------------------------
unsigned int vtkUniformGridAMRIterator::GetCurrentIndex() const
{
  if (this->Reverse)
  {
    vtkErrorMacro("CurrentIndex cannot be obtained when iterating in reverse order.");
    return 0;
  }
  return this->CurrentIndex;
}

//----------------------------------------------------------------
vtkInformation* vtkUniformGridAMRIterator::GetCurrentMetaData()
{
  vtkInformation* info = this->Superclass::GetCurrentMetaData();

  vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(this->GetDataSet());
  if (oamr)
  {
    double bounds[6];
    oamr->GetBounds(this->GetCurrentLevel(), this->GetCurrentIndex(), bounds);
    info->Set(vtkDataObject::BOUNDING_BOX(), bounds, 6);
  }
  return info;
}

//----------------------------------------------------------------
bool vtkUniformGridAMRIterator::IsValid() VTK_FUTURE_CONST
{
  if (!this->GetTraverseSubTree())
  {
    vtkErrorMacro("Iterating over an AMR require TraverseSubTreeOn");
    return false;
  }
  if (!this->GetVisitOnlyLeaves())
  {
    vtkErrorMacro("Iterating over an AMR require VisitOnlyLeavesOn");
    return false;
  }
  return true;
}

//----------------------------------------------------------------
void vtkUniformGridAMRIterator::CheckItemAndLoopIfNeeded()
{
  // Nothing to do if this is the end
  while (!this->IsDoneWithTraversal())
  {
    vtkDataObject* dObj = this->GetCurrentDataObject();
    if (!dObj && this->GetSkipEmptyNodes())
    {
      // If we skip empty nodes, go to next
      this->CurrentIndex++;
      this->NextInternal();
    }
    else
    {
      if (vtkDataObjectTreeIterator::IsDataObjectTree(dObj))
      {
        // If we reach the child AMR, go up one level
        this->CurrentLevel++;
        this->CurrentIndex = 0;
        this->NextInternal();

        // This is important because we should not count the vtkPartitionedDataSet when iterating
        this->CurrentFlatIndex--;
      }
      else
      {
        // We reached a valid item
        break;
      }
    }
  }
}

VTK_ABI_NAMESPACE_END
