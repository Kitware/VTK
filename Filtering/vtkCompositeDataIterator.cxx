/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataIterator.h"

#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetInternals.h"
#include "vtkObjectFactory.h"

class vtkCompositeDataIterator::vtkInternals
{
public:
  class vtkLocation
    {
    vtkCompositeDataSetInternals::VectorOfDataObjects* VectorPtr;
    vtkCompositeDataSetInternals::Iterator Iter;
    vtkCompositeDataSetInternals::ReverseIterator ReverseIter;
    bool Reverse;
    unsigned int Index;
  public:
    // constructor
    vtkLocation(vtkCompositeDataSetInternals::VectorOfDataObjects& ptr, bool reverse)
      {
      this->Reverse = reverse;
      this->VectorPtr = &ptr;
      this->Index = 0;
      this->Iter = ptr.begin();
      this->ReverseIter = ptr.rbegin();
      }

    unsigned int GetIndex()
      { 
      // Note this will return invalid index if this->IsDoneWithTraversal()
      // return true.
      return this->Reverse?
        (this->VectorPtr->size()-(this->Index+1)): this->Index; 
      }

    void Next()
      {
      this->Index++;
      if (this->Reverse)
        {
        this->ReverseIter++;
        }
      else
        {
        this->Iter++;
        }
      }

    // tells if at end.
    bool IsDoneWithTraversal()
      {
      return this->Reverse? (this->ReverseIter == this->VectorPtr->rend()):
        (this->Iter == this->VectorPtr->end());
      }

    // must not be called if IsDoneWithTraversal() returns true.
    vtkDataObject* Data()
      {
      return this->Reverse? this->ReverseIter->DataObject:
        this->Iter->DataObject;
      }

    // must not be called if IsDoneWithTraversal() returns true.
    vtkInformation* MetaData()
      {
      if (this->Reverse)
        {
        if (!this->ReverseIter->MetaData.GetPointer())
          {
          this->ReverseIter->MetaData.TakeReference(vtkInformation::New());
          }
        return this->ReverseIter->MetaData;
        }
      else
        {
        if (!this->Iter->MetaData.GetPointer())
          {
          this->Iter->MetaData.TakeReference(vtkInformation::New());
          }
        return this->Iter->MetaData;
        }
      }

    // must not be called if IsDoneWithTraversal() returns true.
    int HasMetaData()
      {
      return this->Reverse?
        (this->ReverseIter->MetaData.GetPointer() != 0):
        (this->Iter->MetaData.GetPointer() != 0);
      }
    };

  // LocationStack is used to inorder traversal of the tree. As we go down the
  // tree, the stack depth increases. 
  vtkstd::vector<vtkLocation> LocationStack;

  // If Top of the stack has reached its end, the we pop it out and advance the
  // new top of the stack. Note that the new top of the stack (if present) will
  // always be a vtkCompositeDataSet.
  void EnsureStackValidity()
    {
    if (this->LocationStack.size() > 0)
      {
      if (this->LocationStack.back().IsDoneWithTraversal())
        {
        this->LocationStack.pop_back();
        if (this->LocationStack.size() > 0)
          {
          this->LocationStack.back().Next();
          this->EnsureStackValidity();
          }
        }
      }
    }

  // Returns the unique index for the current iterator location.
  vtkCompositeDataSetIndex GetCurrentIndex()
    {
    vtkCompositeDataSetIndex vec;
    vtkstd::vector<vtkLocation>::iterator iter = this->LocationStack.begin();
    for (; iter != this->LocationStack.end(); ++iter)
      {
      vec.push_back(iter->GetIndex());
      }
    return vec;
    }
};

vtkStandardNewMacro(vtkCompositeDataIterator);
vtkCxxRevisionMacro(vtkCompositeDataIterator, "1.4");
//----------------------------------------------------------------------------
vtkCompositeDataIterator::vtkCompositeDataIterator()
{
  this->Reverse = 0;
  this->DataSet = 0;
  this->VisitOnlyLeaves = 1;
  this->TraverseSubTree = 1;
  this->CurrentFlatIndex = 1;
  this->SkipEmptyNodes = 1;
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator::~vtkCompositeDataIterator()
{
  this->SetDataSet(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::SetDataSet(vtkCompositeDataSet* ds)
{
  vtkSetObjectBodyMacro(DataSet, vtkCompositeDataSet, ds);
  if (this->DataSet)
    {
    this->GoToFirstItem();
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::InitTraversal()
{
  this->Reverse = 0;
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::InitReverseTraversal()
{
  this->Reverse = 1;
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::GoToFirstItem()
{
  this->CurrentFlatIndex = 1;
  this->Internals->LocationStack.clear();
  if (!this->DataSet)
    {
    vtkErrorMacro("DataSet must be specified.");
    return;
    }

  vtkInternals::vtkLocation loc(this->DataSet->Internals->Children,
    this->Reverse);
  this->Internals->LocationStack.push_back(loc);

  this->Internals->EnsureStackValidity();

  // We need to ensure that the current data object is non-null. Also, if
  // this->VisitOnlyLeaves is true, we additionally need to ensure that the
  // current item is not a vtkCompositeDataSet.
  while (!this->IsDoneWithTraversal())
    {
    vtkDataObject* dObj = this->Internals->LocationStack.back().Data();
    if ((this->SkipEmptyNodes && !dObj) || 
      (this->VisitOnlyLeaves && dObj->IsA("vtkCompositeDataSet")))
      {
      this->NextInternal();
      }
    else
      {
      break;
      }
    }
}

//----------------------------------------------------------------------------
int vtkCompositeDataIterator::IsDoneWithTraversal()
{
  return (this->Internals->LocationStack.size() == 0);
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::GoToNextItem()
{
  if (!this->IsDoneWithTraversal())
    {
    this->NextInternal();

    // We need to ensure that the current data object is non-null. Also, if
    // this->VisitOnlyLeaves is true, we additionally need to ensure that the
    // current item is not a vtkCompositeDataSet.
    while (!this->IsDoneWithTraversal())
      {
      vtkDataObject* dObj = this->Internals->LocationStack.back().Data();
      if ((this->SkipEmptyNodes && !dObj) || 
        (this->VisitOnlyLeaves && dObj->IsA("vtkCompositeDataSet")))
        {
        this->NextInternal();
        }
      else
        {
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
// Takes the current location to the next dataset. This traverses the tree in
// preorder fashion.
// If the current location is a composite dataset, next is its 1st child dataset.
// If the current is not a composite dataset, then next is the next dataset.
// This method gives no guarantees  whether the current dataset will be
// non-null or leaf.
void vtkCompositeDataIterator::NextInternal()
{
  if (this->Internals->LocationStack.size() > 0)
    {
    vtkInternals::vtkLocation& loc = this->Internals->LocationStack.back();
    if (!loc.IsDoneWithTraversal() && loc.Data() && loc.Data()->IsA("vtkCompositeDataSet"))
      {
      // We've hit a composite dataset, iterate into it unless
      // this->TraverseSubTree == 0.
      if (this->TraverseSubTree)
        {
        vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(loc.Data());
        vtkInternals::vtkLocation subLoc(cds->Internals->Children, this->Reverse);
        this->Internals->LocationStack.push_back(subLoc);
        }
      else
        {
        loc.Next();
        }
      }
    else
      {
      loc.Next();
      }
    this->Internals->EnsureStackValidity();
    this->CurrentFlatIndex++;
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataIterator::GetCurrentDataObject()
{
  if (!this->IsDoneWithTraversal())
    {
    return this->Internals->LocationStack.back().Data();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkCompositeDataIterator::GetCurrentMetaData()
{
  if (!this->IsDoneWithTraversal())
    {
    return this->Internals->LocationStack.back().MetaData();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkCompositeDataIterator::HasCurrentMetaData()
{
  if (!this->IsDoneWithTraversal())
    {
    return this->Internals->LocationStack.back().HasMetaData();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSetIndex vtkCompositeDataIterator::GetCurrentIndex()
{
  return this->Internals->GetCurrentIndex();
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisitOnlyLeaves: "
    << (this->VisitOnlyLeaves? "On" : "Off") << endl;
  os << indent << "Reverse: "
    << (this->Reverse? "On" : "Off") << endl;
  os << indent << "DataSet: " << this->DataSet << endl;
  os << indent << "TraverseSubTree: " 
    << (this->TraverseSubTree? "On" : "Off") << endl;
  os << indent << "SkipEmptyNodes: " 
    << (this->SkipEmptyNodes? "On" : "Off") << endl;
  os << indent << "CurrentFlatIndex: " << this->CurrentFlatIndex << endl;
}

