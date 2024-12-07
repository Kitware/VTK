// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperationStateEntry_h
#define vtkDGOperationStateEntry_h

#include "vtkCompiler.h" // For export macro.
#include "vtkDGOperationState.h"
#include "vtkType.h" // For vtkTypeUInt64.

#include <functional> // For std::function<>.
#include <memory>     // For std::unique_ptr<>.

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkDoubleArray;

/// Signature for a method to evaluate data on a single vtkDGCell::Source instance.
template <typename InputIterator, typename OutputIterator>
using vtkDGCellRangeEvaluator = std::function<void(
  InputIterator& inIter, OutputIterator& outIter, vtkTypeUInt64 begin, vtkTypeUInt64 end)>;

/**@class vtkDGOperationStateEntryBase
 *
 * This is a base class that exists so that vtkDGOperationState can provide a virtual
 * CloneInto method that accepts any of the templated subclasses below.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGOperationStateEntryBase
{
public:
  virtual ~vtkDGOperationStateEntryBase() = default;
};

/**@class vtkDGOperationStateEntry
 * Encapsulate the state required to evaluate DG cell-attributes.
 *
 */
template <typename InputIterator, typename OutputIterator>
class VTK_ALWAYS_EXPORT vtkDGOperationStateEntry : public vtkDGOperationStateEntryBase
{
public:
  vtkDGOperationStateEntry() = default;
  vtkDGOperationStateEntry(const vtkDGOperationStateEntry<InputIterator, OutputIterator>& other)
  {
    other.State->CloneInto(*this);
  }

  vtkDGOperationStateEntry<InputIterator, OutputIterator>& operator=(
    vtkDGOperationStateEntry<InputIterator, OutputIterator>&& other) = default;
  vtkDGOperationStateEntry<InputIterator, OutputIterator>& operator=(
    const vtkDGOperationStateEntry<InputIterator, OutputIterator>& other)
  {
    // CloneInto should also set this->Function.
    other.State->CloneInto(*this);
    return *this;
  }

  std::unique_ptr<vtkDGOperationState> State;
  vtkDGCellRangeEvaluator<InputIterator, OutputIterator> Function;
};

VTK_ABI_NAMESPACE_END

#endif // vtkDGOperationStateEntry_h
// VTK-HeaderTest-Exclude: vtkDGOperationStateEntry.h
