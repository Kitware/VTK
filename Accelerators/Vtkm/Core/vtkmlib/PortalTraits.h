// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_PortalTraits_h
#define vtkmlib_PortalTraits_h

#include "vtkmConfigCore.h" //required for general vtkm setup

#include <vtkm/Types.h>
#include <vtkm/internal/Assume.h>

#include <type_traits>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

struct vtkPortalOfVecOfVecValues;
struct vtkPortalOfVecOfValues;
struct vtkPortalOfScalarValues;

template <typename T>
struct vtkPortalTraits
{
  using TagType = vtkPortalOfScalarValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = ComponentType;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 1;

  static inline void SetComponent(Type& t, vtkm::IdComponent, const ComponentType& v) { t = v; }

  static inline ComponentType GetComponent(const Type& t, vtkm::IdComponent) { return t; }
};

template <typename T, int N>
struct vtkPortalTraits<vtkm::Vec<T, N>>
{
  using TagType = vtkPortalOfVecOfValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec<T, N>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = N;

  static inline void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
  {
    VTKM_ASSUME((i >= 0 && i < N));
    t[i] = v;
  }

  static inline ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
  {
    VTKM_ASSUME((i >= 0 && i < N));
    return t[i];
  }
};

template <typename T, int N>
struct vtkPortalTraits<const vtkm::Vec<T, N>>
{
  using TagType = vtkPortalOfVecOfValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec<T, N>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = N;

  static inline void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
  {
    VTKM_ASSUME((i >= 0 && i < N));
    t[i] = v;
  }

  static inline ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
  {
    VTKM_ASSUME((i >= 0 && i < N));
    return t[i];
  }
};

template <typename T, int N, int M>
struct vtkPortalTraits<vtkm::Vec<vtkm::Vec<T, N>, M>>
{
  using TagType = vtkPortalOfVecOfVecValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec<vtkm::Vec<T, N>, M>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = N * M;

  static constexpr vtkm::IdComponent NUM_COMPONENTS_OUTER = M;
  static constexpr vtkm::IdComponent NUM_COMPONENTS_INNER = N;

  static inline void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
  {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const vtkm::IdComponent j = i % N;
    t[i / N][j] = v;
  }

  static inline ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
  {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const vtkm::IdComponent j = i % N;
    return t[i / N][j];
  }
};

template <typename T, int N, int M>
struct vtkPortalTraits<const vtkm::Vec<vtkm::Vec<T, N>, M>>
{
  using TagType = vtkPortalOfVecOfVecValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec<vtkm::Vec<T, N>, M>;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = N * M;

  static constexpr vtkm::IdComponent NUM_COMPONENTS_OUTER = M;
  static constexpr vtkm::IdComponent NUM_COMPONENTS_INNER = N;

  static inline void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
  {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const vtkm::IdComponent j = i % N;
    t[i / N][j] = v;
  }

  static inline ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
  {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const vtkm::IdComponent j = i % N;
    return t[i / N][j];
  }
};

VTK_ABI_NAMESPACE_END
} // namespace vtkmlib

#endif // vtkmlib_PortalsTraits_h
