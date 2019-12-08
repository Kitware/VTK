/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSetRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkCompositeDataSetRange_h
#define vtkCompositeDataSetRange_h

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetNodeReference.h"
#include "vtkMeta.h"
#include "vtkRange.h"
#include "vtkSmartPointer.h"

#include <cassert>

#ifndef __VTK_WRAP__

namespace vtk
{

// Pass these to vtk::Range(cds, options):
enum class CompositeDataSetOptions : unsigned int
{
  None = 0,
  SkipEmptyNodes = 1 << 1 // Skip null datasets.
};

} // end namespace vtk (for bitflag op definition)

VTK_GENERATE_BITFLAG_OPS(vtk::CompositeDataSetOptions)

namespace vtk
{

namespace detail
{

struct CompositeDataSetRange;
struct CompositeDataSetIterator;

using CompositeDataSetIteratorReference =
  vtk::CompositeDataSetNodeReference<vtkCompositeDataIterator, CompositeDataSetIterator>;

//------------------------------------------------------------------------------
// vtkCompositeDataSet iterator. Returns vtk::CompositeDataSetNodeReference.
struct CompositeDataSetIterator
  : public std::iterator<std::forward_iterator_tag, vtkDataObject*, int,
      CompositeDataSetIteratorReference, CompositeDataSetIteratorReference>
{
private:
  using Superclass = std::iterator<std::forward_iterator_tag, vtkDataObject*, int,
    CompositeDataSetIteratorReference, CompositeDataSetIteratorReference>;
  using InternalIterator = vtkCompositeDataIterator;
  using SmartIterator = vtkSmartPointer<InternalIterator>;

public:
  using iterator_category = typename Superclass::iterator_category;
  using value_type = typename Superclass::value_type;
  using difference_type = typename Superclass::difference_type;
  using pointer = typename Superclass::pointer;
  using reference = typename Superclass::reference;

  CompositeDataSetIterator(const CompositeDataSetIterator& o)
    : Iterator(o.Iterator ? SmartIterator::Take(o.Iterator->NewInstance()) : nullptr)
  {
    this->CopyState(o.Iterator);
  }

  CompositeDataSetIterator(CompositeDataSetIterator&&) noexcept = default;

  CompositeDataSetIterator& operator=(const CompositeDataSetIterator& o)
  {
    this->Iterator = o.Iterator ? SmartIterator::Take(o.Iterator->NewInstance()) : nullptr;
    this->CopyState(o.Iterator);
    return *this;
  }

  CompositeDataSetIterator& operator++() // prefix
  {
    this->Increment();
    return *this;
  }

  CompositeDataSetIterator operator++(int) // postfix
  {
    CompositeDataSetIterator other(*this);
    this->Increment();
    return other;
  }

  reference operator*() const { return this->GetData(); }

  pointer operator->() const { return this->GetData(); }

  friend bool operator==(const CompositeDataSetIterator& lhs, const CompositeDataSetIterator& rhs)
  {
    // A null internal iterator means it is an 'end' sentinal.
    InternalIterator* l = lhs.Iterator;
    InternalIterator* r = rhs.Iterator;

    if (!r && !l)
    { // end == end
      return true;
    }
    else if (!r)
    { // right is end
      return l->IsDoneWithTraversal() != 0;
    }
    else if (!l)
    { // left is end
      return r->IsDoneWithTraversal() != 0;
    }
    else
    { // Both iterators are valid, check unique idx:
      return r->GetCurrentFlatIndex() == l->GetCurrentFlatIndex();
    }
  }

  friend bool operator!=(const CompositeDataSetIterator& lhs, const CompositeDataSetIterator& rhs)
  {
    return !(lhs == rhs); // let the compiler handle this one =)
  }

  friend void swap(CompositeDataSetIterator& lhs, CompositeDataSetIterator& rhs) noexcept
  {
    using std::swap;
    swap(lhs.Iterator, rhs.Iterator);
  }

  friend struct CompositeDataSetRange;

protected:
  // Note: This takes ownership of iter and manages its lifetime.
  // Iter should not be used past this point by the caller.
  CompositeDataSetIterator(SmartIterator&& iter) noexcept : Iterator(std::move(iter)) {}

  // Note: Iterators constructed using this ctor will be considered
  // 'end' iterators via a sentinal pattern.
  CompositeDataSetIterator() noexcept : Iterator(nullptr) {}

private:
  void CopyState(InternalIterator* source)
  {
    if (source)
    {
      assert(this->Iterator != nullptr);
      this->Iterator->SetDataSet(source->GetDataSet());
      this->Iterator->SetSkipEmptyNodes(source->GetSkipEmptyNodes());
      this->Iterator->InitTraversal();
      this->AdvanceTo(source->GetCurrentFlatIndex());
    }
  }

  void AdvanceTo(const unsigned int flatIdx)
  {
    assert(this->Iterator != nullptr);
    assert(this->Iterator->GetCurrentFlatIndex() <= flatIdx);
    while (this->Iterator->GetCurrentFlatIndex() < flatIdx)
    {
      this->Increment();
    }
  }

  void Increment()
  {
    assert(this->Iterator != nullptr);
    assert(!this->Iterator->IsDoneWithTraversal());
    this->Iterator->GoToNextItem();
  }

  CompositeDataSetIteratorReference GetData() const
  {
    assert(this->Iterator != nullptr);
    assert(!this->Iterator->IsDoneWithTraversal());
    return CompositeDataSetIteratorReference{ this->Iterator };
  }

  mutable SmartIterator Iterator;
};

//------------------------------------------------------------------------------
// CompositeDataSet range proxy.
// The const_iterators/references are the same as the non-const versions, since
// vtkObjects marked const are unusable.
struct CompositeDataSetRange
{
private:
  using InternalIterator = vtkCompositeDataIterator;
  using SmartIterator = vtkSmartPointer<InternalIterator>;

public:
  using size_type = int;
  using iterator = CompositeDataSetIterator;
  using const_iterator = CompositeDataSetIterator;
  using reference = CompositeDataSetIteratorReference;
  using const_reference = const CompositeDataSetIteratorReference;
  using value_type = vtkDataObject*;

  CompositeDataSetRange(
    vtkCompositeDataSet* cds, CompositeDataSetOptions opts = CompositeDataSetOptions::None)
    : CompositeDataSet(cds)
    , Options(opts)
  {
    assert(this->CompositeDataSet);
  }

  vtkCompositeDataSet* GetCompositeDataSet() const noexcept { return this->CompositeDataSet; }

  CompositeDataSetOptions GetOptions() const noexcept { return this->Options; }

  // This is O(N), since the size requires traversal due to various options.
  size_type size() const
  {
    size_type result = 0;
    auto iter = this->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      ++result;
      iter->GoToNextItem();
    }
    return result;
  }

  iterator begin() const { return CompositeDataSetIterator{ this->NewIterator() }; }

  iterator end() const { return CompositeDataSetIterator{}; }

  // Note: These return mutable objects because const vtkObject are unusable.
  const_iterator cbegin() const { return CompositeDataSetIterator{ this->NewIterator() }; }

  // Note: These return mutable objects because const vtkObjects are unusable.
  const_iterator cend() const { return CompositeDataSetIterator{}; }

private:
  SmartIterator NewIterator() const
  {
    using Opts = vtk::CompositeDataSetOptions;

    auto result = SmartIterator::Take(this->CompositeDataSet->NewIterator());
    result->SetSkipEmptyNodes((this->Options & Opts::SkipEmptyNodes) != Opts::None);
    result->InitTraversal();
    return result;
  }

  mutable vtkSmartPointer<vtkCompositeDataSet> CompositeDataSet;
  CompositeDataSetOptions Options;
};

}
} // end namespace vtk::detail

#endif // __VTK_WRAP__

#endif // vtkCompositeDataSetRange_h

// VTK-HeaderTest-Exclude: vtkCompositeDataSetRange.h
