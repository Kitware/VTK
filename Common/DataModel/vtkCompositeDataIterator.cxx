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
  // This implements a simple, no frills, depth-first iterator that iterates
  // over the composite dataset.
  class vtkIterator
    {
    vtkDataObject* DataObject;
    vtkCompositeDataSet* CompositeDataSet;

    vtkCompositeDataSetInternals::Iterator Iter;
    vtkCompositeDataSetInternals::ReverseIterator ReverseIter;
    vtkIterator* ChildIterator;
    
    vtkInternals* Parent;
    bool Reverse;
    bool PassSelf;
    unsigned int ChildIndex;

    void InitChildIterator()
      {
      if (!this->ChildIterator)
        {
        this->ChildIterator = new vtkIterator(this->Parent);
        }
      this->ChildIterator->Initialize(this->Reverse, 0);

      if (this->Reverse && 
        this->ReverseIter != this->GetInternals(this->CompositeDataSet)->Children.rend())
        {
        this->ChildIterator->Initialize(this->Reverse, 
          this->ReverseIter->DataObject);
        }
      else if (!this->Reverse &&
        this->Iter != this->GetInternals(this->CompositeDataSet)->Children.end())
        {
        this->ChildIterator->Initialize(this->Reverse, 
          this->Iter->DataObject);
        }
      }

    vtkCompositeDataSetInternals* GetInternals(vtkCompositeDataSet* cd)
      {
      return this->Parent->GetInternals(cd);
      }
  public:
    vtkIterator(vtkInternals* parent)
      {
      this->ChildIterator = 0;
      this->Parent = parent;
      }

    ~vtkIterator()
      {
      delete this->ChildIterator;
      this->ChildIterator = 0;
      }



    void Initialize(bool reverse, vtkDataObject* dataObj)
      {
      vtkCompositeDataSet* compositeData = vtkCompositeDataSet::SafeDownCast(dataObj);
      this->Reverse = reverse;
      this->DataObject = dataObj;
      this->CompositeDataSet = compositeData;
      this->ChildIndex = 0;
      this->PassSelf = true;
      
      delete this->ChildIterator;
      this->ChildIterator = NULL;

      if (compositeData)
        {
        this->Iter = this->GetInternals(compositeData)->Children.begin(); 
        this->ReverseIter = this->GetInternals(compositeData)->Children.rbegin();
        this->InitChildIterator();
        }
      }

    bool InSubTree()
      {
      if (this->PassSelf || this->IsDoneWithTraversal())
        {
        return false;
        }

      if (!this->ChildIterator)
        {
        return false;
        }

      if (this->ChildIterator->PassSelf)
        {
        return false;
        }

      return true;
      }


    bool IsDoneWithTraversal()
      {
      if (!this->DataObject)
        {
        return true;
        }

      if (this->PassSelf)
        {
        return false;
        }

      if (!this->CompositeDataSet)
        {
        return true;
        }

      if (this->Reverse && 
        this->ReverseIter == this->GetInternals(this->CompositeDataSet)->Children.rend())
        {
        return true;
        }

      if (!this->Reverse &&
        this->Iter == this->GetInternals(this->CompositeDataSet)->Children.end())
        {
        return true;
        }
      return false;
      }

    // Should not be called is this->IsDoneWithTraversal() returns true.
    vtkDataObject* GetCurrentDataObject()
      {
      if (this->PassSelf)
        {
        return this->DataObject;
        }
      return this->ChildIterator?
        this->ChildIterator->GetCurrentDataObject() : 0;
      }

    vtkInformation* GetCurrentMetaData()
      {
      if (this->PassSelf || !this->ChildIterator)
        {
        return NULL;
        }

      if (this->ChildIterator->PassSelf)
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
      return this->ChildIterator->GetCurrentMetaData();
      }

    int HasCurrentMetaData()
      {
      if (this->PassSelf || !this->ChildIterator)
        {
        return 0;
        }

      if (this->ChildIterator->PassSelf)
        {
        return this->Reverse?
          (this->ReverseIter->MetaData.GetPointer() != 0):
          (this->Iter->MetaData.GetPointer() != 0);
        }

      return this->ChildIterator->HasCurrentMetaData();
      }

    // Go to the next element.
    void Next()
      {
      if (this->PassSelf)
        {
        this->PassSelf = false;
        }
      else if (this->ChildIterator)
        {
        this->ChildIterator->Next();
        if (this->ChildIterator->IsDoneWithTraversal())
          {
          this->ChildIndex++;
          if (this->Reverse)
            {
            this->ReverseIter++;
            }
          else
            {
            this->Iter++;
            }
          this->InitChildIterator();
          }
        }
      }

    // Returns the full-tree index for the current location.
    vtkCompositeDataSetIndex GetCurrentIndex()
      {
      vtkCompositeDataSetIndex index;
      if (this->PassSelf || this->IsDoneWithTraversal() || !this->ChildIterator)
        {
        return index;
        }
      index.push_back(this->ChildIndex);
      vtkCompositeDataSetIndex childIndex = this->ChildIterator->GetCurrentIndex();
      index.insert(index.end(), childIndex.begin(), childIndex.end());
      return index;
      }

    };

  // Description:
  // Helper method used by vtkInternals to get access to the internals of
  // vtkCompositeDataSet.
  vtkCompositeDataSetInternals* GetInternals(vtkCompositeDataSet* cd)
    {
    return this->CompositeDataIterator->GetInternals(cd);
    }

  vtkInternals()
    {
    this->Iterator = new vtkIterator(this);
    }
  ~vtkInternals()
    {
    delete this->Iterator;
    this->Iterator = 0;
    }

  vtkIterator *Iterator;
  vtkCompositeDataIterator* CompositeDataIterator;
};

vtkStandardNewMacro(vtkCompositeDataIterator);
//----------------------------------------------------------------------------
vtkCompositeDataIterator::vtkCompositeDataIterator()
{
  this->Reverse = 0;
  this->DataSet = 0;
  this->VisitOnlyLeaves = 1;
  this->TraverseSubTree = 1;
  this->CurrentFlatIndex = 0;
  this->SkipEmptyNodes = 1;
  this->Internals = new vtkInternals();
  this->Internals->CompositeDataIterator = this;
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
  this->GoToFirstItem();
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
int vtkCompositeDataIterator::IsDoneWithTraversal()
{
  return this->Internals->Iterator->IsDoneWithTraversal();
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::GoToFirstItem()
{
  this->CurrentFlatIndex = 0;
  this->Internals->Iterator->Initialize(this->Reverse !=0, this->DataSet);
  this->NextInternal();

  while (!this->Internals->Iterator->IsDoneWithTraversal())
    {
    vtkDataObject* dObj = this->Internals->Iterator->GetCurrentDataObject();
    if ((!dObj && this->SkipEmptyNodes) ||
      (this->VisitOnlyLeaves && vtkCompositeDataSet::SafeDownCast(dObj)))
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
void vtkCompositeDataIterator::GoToNextItem()
{
  if (!this->Internals->Iterator->IsDoneWithTraversal())
    {
    this->NextInternal();

    while (!this->Internals->Iterator->IsDoneWithTraversal())
      {
      vtkDataObject* dObj = this->Internals->Iterator->GetCurrentDataObject();
      if ((!dObj && this->SkipEmptyNodes) ||
        (this->VisitOnlyLeaves && vtkCompositeDataSet::SafeDownCast(dObj)))
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
void vtkCompositeDataIterator::NextInternal()
{
  do
    {
    this->CurrentFlatIndex++;
    this->Internals->Iterator->Next();
    }
  while (!this->TraverseSubTree && this->Internals->Iterator->InSubTree());
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataIterator::GetCurrentDataObject()
{
  if (!this->IsDoneWithTraversal())
    {
    return this->Internals->Iterator->GetCurrentDataObject();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkCompositeDataIterator::GetCurrentMetaData()
{
  if (!this->IsDoneWithTraversal())
    {
    return this->Internals->Iterator->GetCurrentMetaData();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkCompositeDataIterator::HasCurrentMetaData()
{
  if (!this->IsDoneWithTraversal())
    {
    return this->Internals->Iterator->HasCurrentMetaData();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSetIndex vtkCompositeDataIterator::GetCurrentIndex()
{
  return this->Internals->Iterator->GetCurrentIndex();
}

//----------------------------------------------------------------------------
unsigned int vtkCompositeDataIterator::GetCurrentFlatIndex()
{
  if (this->Reverse)
    {
    vtkErrorMacro(
      "FlatIndex cannot be obtained when iterating in reverse order.");
    return 0;
    }
  return this->CurrentFlatIndex;
}

//----------------------------------------------------------------------------
vtkCompositeDataSetInternals* vtkCompositeDataIterator::GetInternals(
  vtkCompositeDataSet* cd)
{
  if (cd)
    {
    return cd->Internals;
    }

  return 0;
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

