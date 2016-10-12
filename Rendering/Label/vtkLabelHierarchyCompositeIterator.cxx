/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelHierarchyCompositeIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkLabelHierarchyCompositeIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <utility>
#include <vector>

vtkStandardNewMacro(vtkLabelHierarchyCompositeIterator);

class vtkLabelHierarchyCompositeIterator::Internal
{
public:
  typedef std::vector<std::pair<vtkSmartPointer<vtkLabelHierarchyIterator>, int> > IteratorVector;
  IteratorVector Iterators;
  IteratorVector::size_type CurrentIterator;
  IteratorVector::size_type InitialTraversal;
  int CurrentCount;
};

vtkLabelHierarchyCompositeIterator::vtkLabelHierarchyCompositeIterator()
{
  this->Implementation = new Internal();
  this->Implementation->CurrentIterator = 0;
  this->Implementation->InitialTraversal = 0;
  this->Implementation->CurrentCount = 0;
}

vtkLabelHierarchyCompositeIterator::~vtkLabelHierarchyCompositeIterator()
{
  delete this->Implementation;
}

void vtkLabelHierarchyCompositeIterator::AddIterator(vtkLabelHierarchyIterator* it, int count)
{
  this->Implementation->Iterators.push_back(
    std::make_pair(vtkSmartPointer<vtkLabelHierarchyIterator>(it), count));
}

void vtkLabelHierarchyCompositeIterator::ClearIterators()
{
  this->Implementation->Iterators.clear();
}

void vtkLabelHierarchyCompositeIterator::Begin(vtkIdTypeArray* list)
{
  this->Implementation->CurrentIterator = 0;
  this->Implementation->CurrentCount = 0;
  this->Implementation->InitialTraversal = 0;

  // Take care of the no-iterator case
  if (this->Implementation->Iterators.size() == 0)
  {
    return;
  }

  // Call Begin on each child iterator.
  for (Internal::IteratorVector::size_type i = 0; i < this->Implementation->Iterators.size(); ++i)
  {
    this->Implementation->Iterators[i].first->Begin(list);
    if (this->TraversedBounds)
    {
      this->Implementation->Iterators[i].first->SetTraversedBounds(this->TraversedBounds);
    }
  }

  // Find a non-empty iterator to start at if it exists.
  // If not, CurrentIterator will be past the end of the array, signaling IsAtEnd.
  while (this->Implementation->CurrentIterator < this->Implementation->Iterators.size() &&
         this->Implementation->Iterators[this->Implementation->CurrentIterator].first->IsAtEnd())
  {
    this->Implementation->CurrentIterator++;
  }
}

void vtkLabelHierarchyCompositeIterator::Next()
{
  Internal::IteratorVector::size_type numIterators = this->Implementation->Iterators.size();
  Internal::IteratorVector::size_type numTried = 0;
  vtkLabelHierarchyIterator* iter = this->Implementation->Iterators[this->Implementation->CurrentIterator].first;
  int count = this->Implementation->Iterators[this->Implementation->CurrentIterator].second;
  while (numTried <= numIterators && (iter->IsAtEnd() || this->Implementation->CurrentCount >= count))
  {
    this->Implementation->CurrentCount = 0;
    this->Implementation->CurrentIterator = (this->Implementation->CurrentIterator + 1) % numIterators;
    iter = this->Implementation->Iterators[this->Implementation->CurrentIterator].first;
    count = this->Implementation->Iterators[this->Implementation->CurrentIterator].second;
    if (!iter->IsAtEnd())
    {
      // Don't call Next() if this is the first time we are traversing this iterator.
      if (this->Implementation->InitialTraversal < this->Implementation->CurrentIterator)
      {
        this->Implementation->InitialTraversal = this->Implementation->CurrentIterator;
      }
      else
      {
        iter->Next();
      }
    }
    ++numTried;
  }
  this->Implementation->CurrentCount++;
  if (numTried > numIterators)
  {
    // Signal end of iterator.
    this->Implementation->CurrentIterator = numIterators;
  }
}

bool vtkLabelHierarchyCompositeIterator::IsAtEnd()
{
  return this->Implementation->CurrentIterator >= this->Implementation->Iterators.size();
}

vtkIdType vtkLabelHierarchyCompositeIterator::GetLabelId()
{
  if (this->Implementation->CurrentIterator < this->Implementation->Iterators.size())
  {
    return this->Implementation->Iterators[this->Implementation->CurrentIterator].first->GetLabelId();
  }
  return -1;
}

vtkLabelHierarchy* vtkLabelHierarchyCompositeIterator::GetHierarchy()
{
  if (this->Implementation->CurrentIterator < this->Implementation->Iterators.size())
  {
    return this->Implementation->Iterators[this->Implementation->CurrentIterator].first->GetHierarchy();
  }
  return 0;
}

void vtkLabelHierarchyCompositeIterator::GetNodeGeometry(double ctr[3], double& size)
{
  if (this->Implementation->CurrentIterator < this->Implementation->Iterators.size())
  {
    this->Implementation->Iterators[this->Implementation->CurrentIterator].first->GetNodeGeometry(ctr, size);
  }
}

void vtkLabelHierarchyCompositeIterator::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
