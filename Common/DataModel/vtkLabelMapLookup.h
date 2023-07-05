// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLabelMapLookup
 * @brief   provide an efficient numeric label lookup
 *
 *
 * vtkLabelMapLookup is a light weight helper object that enables faster
 * lookup of a segmentation label from a set of labels. It uses caching, and
 * different strategies depending on the size of the set of labels.
 *
 * Note that, due to speed concerns, vtkLabelMapLookup does not inherit from
 * vtkObject hence does not support the usual VTK reference counting.
 *
 * @sa
 * vtkSurfaceNets2D vtkSurfaceNets3D vtkDiscreteFlyingEdgesClipper2D
 */

#ifndef vtkLabelMapLookup_h
#define vtkLabelMapLookup_h

#include "vtkCommonDataModelModule.h"

#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
// Determine whether an image label/object has been specified for output.
// This requires looking up an image pixel/scalar value and determining
// whether it's part of a segmented object. Since this can be relatively
// expensive when performed many times, different lookup classes are used
// depending on the number of labels specified. A cache is used for the
// common case of repeated queries for the same label value.
template <typename T>
struct vtkLabelMapLookup
{
  T CachedValue;
  T CachedOutValue;
  bool CachedOutValueInitialized;

  vtkLabelMapLookup(const double* values, int vtkNotUsed(numValues))
  {
    this->CachedValue = static_cast<T>(values[0]);
    this->CachedOutValue = static_cast<T>(values[0]);
    this->CachedOutValueInitialized = false;
  }
  virtual ~vtkLabelMapLookup() = default;
  virtual bool IsLabelValue(T label) = 0;
  bool IsLabelValueInCache(T label, bool& inLabelSet)
  {
    if (label == this->CachedValue)
    {
      inLabelSet = true;
      return true;
    }
    else if (this->CachedOutValueInitialized && label == this->CachedOutValue)
    {
      inLabelSet = false;
      return true;
    }
    else
    {
      return false;
    }
  }

  // A factory method for creating the right type of label map based
  // on the number of labels in the set.
  static vtkLabelMapLookup<T>* CreateLabelLookup(const double* values, vtkIdType numLabels);
}; // vtkLabelMapLookup

// Cache a single contour value
template <typename T>
struct SingleLabelValue : public vtkLabelMapLookup<T>
{
  SingleLabelValue(const double* values)
    : vtkLabelMapLookup<T>(values, 1)
  {
  }
  bool IsLabelValue(T label) override { return label == this->CachedValue; }
}; // SingleLabelValue

// Represent a few contour values with a std::vector<>
template <typename T>
struct LabelVector : public vtkLabelMapLookup<T>
{
  std::vector<T> Map;

  LabelVector(const double* values, int numValues)
    : vtkLabelMapLookup<T>(values, numValues)
  {
    for (int vidx = 0; vidx < numValues; vidx++)
    {
      Map.push_back(static_cast<T>(values[vidx]));
    }
  }
  bool IsLabelValue(T label) override
  {
    bool inLabelSet;
    // Check the cache
    if (this->IsLabelValueInCache(label, inLabelSet))
    {
      return inLabelSet;
    }

    // Not in the cache, check the vector
    if (std::find(this->Map.begin(), this->Map.end(), label) != this->Map.end())
    {
      this->CachedValue = label;
      return true;
    }
    else
    {
      this->CachedOutValue = label;
      this->CachedOutValueInitialized = true;
      return false;
    }
  }
}; // LabelVector

// Represent many contour values with a std::set<>
template <typename T>
struct LabelSet : public vtkLabelMapLookup<T>
{
  std::unordered_set<T> Map;

  LabelSet(const double* values, int numValues)
    : vtkLabelMapLookup<T>(values, numValues)
  {
    for (int vidx = 0; vidx < numValues; vidx++)
    {
      Map.insert(static_cast<T>(values[vidx]));
    }
  }
  bool IsLabelValue(T label) override
  {
    bool inLabelSet;
    // Check the cache
    if (this->IsLabelValueInCache(label, inLabelSet))
    {
      return inLabelSet;
    }

    // Not in cache, check the map
    if (this->Map.find(label) != this->Map.end())
    {
      this->CachedValue = label;
      return true;
    }
    else
    {
      this->CachedOutValue = label;
      this->CachedOutValueInitialized = true;
      return false;
    }
  }
}; // LabelSet

// Given a list of label values (represented generically as doubles),
// create the appropriate lookup class and add the label values to
// the collection of labels.
template <typename T>
vtkLabelMapLookup<T>* vtkLabelMapLookup<T>::CreateLabelLookup(
  const double* values, vtkIdType numLabels)
{
  // These cutoffs are empirical and can be changed.
  if (numLabels == 1)
  {
    return new SingleLabelValue<T>(values);
  }
  else if (numLabels < 20)
  {
    return new LabelVector<T>(values, numLabels);
  }
  else
  {
    return new LabelSet<T>(values, numLabels);
  }
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkLabelMapLookup.h
