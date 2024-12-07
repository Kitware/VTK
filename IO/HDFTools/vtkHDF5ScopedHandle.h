// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkHDF5ScopedHandle_h
#define vtkHDF5ScopedHandle_h

#include "vtk_hdf5.h"

#include <algorithm>

namespace vtkHDF
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * RAII class for automatically closing H5 handles.
 */
#define DefineScopedHandle(name)                                                                   \
  class ScopedH5##name##Handle                                                                     \
  {                                                                                                \
  public:                                                                                          \
    ScopedH5##name##Handle()                                                                       \
      : Handle(H5I_INVALID_HID)                                                                    \
    {                                                                                              \
    }                                                                                              \
    ScopedH5##name##Handle(ScopedH5##name##Handle&& other) noexcept                                \
    {                                                                                              \
      this->Handle = other.Handle;                                                                 \
      other.Handle = H5I_INVALID_HID;                                                              \
    }                                                                                              \
    ScopedH5##name##Handle& operator=(ScopedH5##name##Handle&& other) noexcept                     \
    {                                                                                              \
      std::swap(this->Handle, other.Handle);                                                       \
      return *this;                                                                                \
    }                                                                                              \
    ScopedH5##name##Handle(const ScopedH5##name##Handle& other) = delete;                          \
    ScopedH5##name##Handle& operator=(const ScopedH5##name##Handle&) = delete;                     \
    ScopedH5##name##Handle(hid_t handle)                                                           \
      : Handle(handle)                                                                             \
    {                                                                                              \
    }                                                                                              \
    virtual ~ScopedH5##name##Handle()                                                              \
    {                                                                                              \
      if (this->Handle >= 0)                                                                       \
      {                                                                                            \
        H5##name##close(this->Handle);                                                             \
      }                                                                                            \
    }                                                                                              \
                                                                                                   \
    operator hid_t() const                                                                         \
    {                                                                                              \
      return this->Handle;                                                                         \
    }                                                                                              \
                                                                                                   \
  private:                                                                                         \
    hid_t Handle;                                                                                  \
  };

// Defines ScopedH5AHandle closed with H5Aclose
DefineScopedHandle(A);

// Defines ScopedH5DHandle closed with H5Dclose
DefineScopedHandle(D);

// Defines ScopedH5FHandle closed with H5Fclose
DefineScopedHandle(F);

// Defines ScopedH5GHandle closed with H5Gclose
DefineScopedHandle(G);

// Defines ScopedH5PHandle closed with H5Pclose
DefineScopedHandle(P);

// Defines ScopedH5SHandle closed with H5Sclose
DefineScopedHandle(S);

// Defines ScopedH5THandle closed with H5Tclose
DefineScopedHandle(T);

VTK_ABI_NAMESPACE_END
}

#endif
// VTK-HeaderTest-Exclude: vtkHDF5ScopedHandle.h
