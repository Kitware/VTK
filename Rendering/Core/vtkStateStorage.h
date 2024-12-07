// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStateStorage
 * @brief   Class to make storing and comparing state quick and easy
 *
 * vtkStateStorage is just a thin wrapper around std::vector<unsigned char>
 * It is best to use this class as an ivar so that allocs do not happen
 * too often.
 *
 * Example usage:
 * @code
 *
 * // compute the new state in a temp ivar
 * // note that clear does not free memory
 * this->TempState.Clear();
 * this->TempState.Append(act->GetProperty()->GetMTime(), "property mtime");
 * this->TempState.Append(
 *   this->CurrentInput ? this->CurrentInput->GetMTime() : 0, "input mtime");
 * this->TempState.Append(
 *   act->GetTexture() ? act->GetTexture()->GetMTime() : 0, "texture mtime");
 *
 * // now compare against the last state value
 *
 * if (this->VBOBuildState != this->TempState)
 * {
 *   // set the ivar to the new state
 *   this->VBOBuildState = this->TempState;
 *   do something...
 * }
 *
 * @endcode
 *
 */

#ifndef vtkStateStorage_h
#define vtkStateStorage_h

#include "vtkABINamespace.h"

#include <algorithm>
#include <string>
#include <vector>

// uncomment the following line to add in state debugging information
// #define USE_STATE_DEBUGGING 1
#ifdef USE_STATE_DEBUGGING

VTK_ABI_NAMESPACE_BEGIN
class vtkStateStorage
{
public:
  vtkStateStorage() {}

  // clear the storage
  void Clear()
  {
    this->Storage.clear();
    this->StorageOffsets.clear();
    this->StorageNames.clear();
  }

  // append a data item to the state
  template <class T>
  void Append(const T& value, const char* name);

  bool operator!=(const vtkStateStorage& b) const
  {
    // for debug we also lookup the name of what was different
    this->WhatWasDifferent = "";
    if (this->Storage.size() != b.Storage.size())
    {
      this->WhatWasDifferent = "Different state sizes";
      return true;
    }
    for (size_t i = 0; i < this->Storage.size(); ++i)
    {
      if (this->Storage[i] != b.Storage[i])
      {
        size_t block = 0;
        while (this->StorageOffsets.size() > block + 1 && this->StorageOffsets[block + 1] >= i)
        {
          block++;
        }
        this->WhatWasDifferent = this->StorageNames[block] + " was different";
        return true;
      }
    }
    return false;
  }

  vtkStateStorage& operator=(const vtkStateStorage& b)
  {
    this->Storage = b.Storage;
    this->StorageNames = b.StorageNames;
    this->StorageOffsets = b.StorageOffsets;
    return *this;
  }

protected:
  std::vector<unsigned char> Storage;
  std::vector<std::string> StorageNames;
  std::vector<size_t> StorageOffsets;
  mutable std::string WhatWasDifferent;

private:
  vtkStateStorage(const vtkStateStorage&) = delete;
};

template <class T>
inline void vtkStateStorage::Append(const T& value, const char* name)
{
  this->StorageOffsets.push_back(this->Storage.size());
  this->StorageNames.push_back(name);
  const char* start = reinterpret_cast<const char*>(&value);
  this->Storage.insert(this->Storage.end(), start, start + sizeof(T));
}

VTK_ABI_NAMESPACE_END
#else  // normal implementation

VTK_ABI_NAMESPACE_BEGIN
class vtkStateStorage
{
public:
  vtkStateStorage() = default;

  // clear the storage
  void Clear() { this->Storage.clear(); }

  // append a data item to the state
  template <class T>
  void Append(const T& value, const char* name);

  bool operator!=(const vtkStateStorage& b) const { return this->Storage != b.Storage; }

  vtkStateStorage& operator=(const vtkStateStorage& b) = default;

protected:
  std::vector<unsigned char> Storage;

private:
  vtkStateStorage(const vtkStateStorage&) = delete;
};

template <class T>
inline void vtkStateStorage::Append(const T& value, const char*)
{
  const char* start = reinterpret_cast<const char*>(&value);
  this->Storage.insert(this->Storage.end(), start, start + sizeof(T));
}

VTK_ABI_NAMESPACE_END
#endif // normal implementation

#endif // vtkStateStorage_h

// VTK-HeaderTest-Exclude: vtkStateStorage.h
