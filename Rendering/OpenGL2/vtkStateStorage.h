/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingOpenGL2Module.h" // for export macro

#include <algorithm>
#include <string>

#ifndef NDEBUG // a debug implementation

class VTKRENDERINGOPENGL2_EXPORT vtkStateStorage
{
public:
  vtkStateStorage() {}

  // clear the storage
  void Clear() {
    this->Storage.clear();
    this->StorageOffsets.clear();
    this->StorageNames.clear();
  }

  // append a data item to the state
  template <class T>
  void Append(const T &value, const char *name);

  bool operator !=(const vtkStateStorage &b) const {
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
        while (this->StorageOffsets.size() > block + 1 && this->StorageOffsets[block+1] >= i)
        {
          block++;
        }
        this->WhatWasDifferent = this->StorageNames[block] + " was different";
        return true;
      }
    }
    return false;
  }

  vtkStateStorage& operator=(const vtkStateStorage&b) {
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
inline void vtkStateStorage::Append(const T &value, const char *name)
{
  this->StorageOffsets.push_back(this->Storage.size());
  this->StorageNames.push_back(name);
  const char *start = reinterpret_cast<const char *>(&value);
  this->Storage.insert(this->Storage.end(), start, start + sizeof(T));
}

#else // release implementation

class VTKRENDERINGOPENGL2_EXPORT vtkStateStorage
{
public:
  vtkStateStorage() {}

  // clear the storage
  void Clear() { this->Storage.clear(); }

  // append a data item to the state
  template <class T>
  void Append(const T &value, const char *name);

  bool operator !=(const vtkStateStorage &b) const {
    return this->Storage != b.Storage;
  }

  vtkStateStorage& operator=(const vtkStateStorage&b) {
    this->Storage = b.Storage;
    return *this;
  }

protected:
  std::vector<unsigned char> Storage;

 private:
  vtkStateStorage(const vtkStateStorage&) = delete;
};

template <class T>
inline void vtkStateStorage::Append(const T &value, const char *)
{
  const char *start = reinterpret_cast<const char *>(&value);
  this->Storage.insert(this->Storage.end(), start, start + sizeof(T));
}

#endif // Release implementation

#endif // vtkStateStorage_h

// VTK-HeaderTest-Exclude: vtkStateStorage.h
