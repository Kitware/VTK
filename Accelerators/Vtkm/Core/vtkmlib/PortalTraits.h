// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_PortalTraits_h
#define vtkmlib_PortalTraits_h

#include "vtkmConfigCore.h" //required for general viskores setup

#include <viskores/Types.h>
#include <viskores/internal/Assume.h>

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
  static constexpr viskores::IdComponent NUM_COMPONENTS = 1;

  static void SetComponent(Type& t, viskores::IdComponent, const ComponentType& v) { t = v; }

  static ComponentType GetComponent(const Type& t, viskores::IdComponent) { return t; }
};

template <typename T, int N>
struct vtkPortalTraits<viskores::Vec<T, N>>
{
  using TagType = vtkPortalOfVecOfValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = viskores::Vec<T, N>;
  static constexpr viskores::IdComponent NUM_COMPONENTS = N;

  static void SetComponent(Type& t, viskores::IdComponent i, const ComponentType& v)
  {
    VISKORES_ASSUME((i >= 0 && i < N));
    t[i] = v;
  }

  static ComponentType GetComponent(const Type& t, viskores::IdComponent i)
  {
    VISKORES_ASSUME((i >= 0 && i < N));
    return t[i];
  }
};

template <typename T, int N>
struct vtkPortalTraits<const viskores::Vec<T, N>>
{
  using TagType = vtkPortalOfVecOfValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = viskores::Vec<T, N>;
  static constexpr viskores::IdComponent NUM_COMPONENTS = N;

  static void SetComponent(Type& t, viskores::IdComponent i, const ComponentType& v)
  {
    VISKORES_ASSUME((i >= 0 && i < N));
    t[i] = v;
  }

  static ComponentType GetComponent(const Type& t, viskores::IdComponent i)
  {
    VISKORES_ASSUME((i >= 0 && i < N));
    return t[i];
  }
};

template <typename T, int N, int M>
struct vtkPortalTraits<viskores::Vec<viskores::Vec<T, N>, M>>
{
  using TagType = vtkPortalOfVecOfVecValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = viskores::Vec<viskores::Vec<T, N>, M>;
  static constexpr viskores::IdComponent NUM_COMPONENTS = N * M;

  static constexpr viskores::IdComponent NUM_COMPONENTS_OUTER = M;
  static constexpr viskores::IdComponent NUM_COMPONENTS_INNER = N;

  static void SetComponent(Type& t, viskores::IdComponent i, const ComponentType& v)
  {
    VISKORES_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const viskores::IdComponent j = i % N;
    t[i / N][j] = v;
  }

  static ComponentType GetComponent(const Type& t, viskores::IdComponent i)
  {
    VISKORES_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const viskores::IdComponent j = i % N;
    return t[i / N][j];
  }
};

template <typename T, int N, int M>
struct vtkPortalTraits<const viskores::Vec<viskores::Vec<T, N>, M>>
{
  using TagType = vtkPortalOfVecOfVecValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = viskores::Vec<viskores::Vec<T, N>, M>;
  static constexpr viskores::IdComponent NUM_COMPONENTS = N * M;

  static constexpr viskores::IdComponent NUM_COMPONENTS_OUTER = M;
  static constexpr viskores::IdComponent NUM_COMPONENTS_INNER = N;

  static void SetComponent(Type& t, viskores::IdComponent i, const ComponentType& v)
  {
    VISKORES_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const viskores::IdComponent j = i % N;
    t[i / N][j] = v;
  }

  static ComponentType GetComponent(const Type& t, viskores::IdComponent i)
  {
    VISKORES_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const viskores::IdComponent j = i % N;
    return t[i / N][j];
  }
};

VTK_ABI_NAMESPACE_END
} // namespace vtkmlib

#endif // vtkmlib_PortalsTraits_h
