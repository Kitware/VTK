//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkmlib_PortalTraits_h
#define vtkmlib_PortalTraits_h

#include "vtkmConfig.h" //required for general vtkm setup

#include <vtkm/Types.h>
#include <vtkm/internal/Assume.h>

#include <type_traits>

namespace tovtkm {

template<typename T>
struct vtkPortalTraits
{
  using ComponentType = typename std::remove_const<T>::type;
  using Type = ComponentType;
  static VTKM_CONSTEXPR vtkm::IdComponent NUM_COMPONENTS =  1;

  static inline
  void SetComponent(Type& t, vtkm::IdComponent, const ComponentType& v)
    { t = v; }

  static inline
  ComponentType GetComponent(const Type& t, vtkm::IdComponent)
    {
    return t;
    }
};

template<typename T, int N>
struct vtkPortalTraits< vtkm::Vec<T,N> >
{
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec<T,N>;
  static VTKM_CONSTEXPR vtkm::IdComponent NUM_COMPONENTS =  N;

  static inline
  void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
    {
    VTKM_ASSUME((i >= 0 && i < N));
    t[i] = v;
    }

  static inline
  ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
    {
    VTKM_ASSUME((i >= 0 && i < N));
    return t[i];
    }
};

template<typename T, int N>
struct vtkPortalTraits< const vtkm::Vec<T,N> >
{
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec<T,N>;
  static VTKM_CONSTEXPR vtkm::IdComponent NUM_COMPONENTS =  N;

  static inline
  void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
    {
    VTKM_ASSUME((i >= 0 && i < N));
    t[i] = v;
    }

  static inline
  ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
    {
    VTKM_ASSUME((i >= 0 && i < N));
    return t[i];
    }
};

template<typename T, int N, int M>
struct vtkPortalTraits<vtkm::Vec< vtkm::Vec<T,N>, M> >
{
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec< vtkm::Vec<T,N>, M>;
  static VTKM_CONSTEXPR vtkm::IdComponent NUM_COMPONENTS =  N*M;

  static inline
  void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
    {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    //We need to convert i back to a 2d index
    const vtkm::IdComponent j = i%N;
    t[i/N][j] = v;
    }

  static inline
  ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
    {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    //We need to convert i back to a 2d index
    const vtkm::IdComponent j = i%N;
    return t[i/N][j];
    }
};

template<typename T, int N, int M>
struct vtkPortalTraits< const vtkm::Vec< vtkm::Vec<T,N>, M> >
{
  using ComponentType = typename std::remove_const<T>::type;
  using Type = vtkm::Vec< vtkm::Vec<T,N>, M>;
  static VTKM_CONSTEXPR vtkm::IdComponent NUM_COMPONENTS =  N*M;

  static inline
  void SetComponent(Type& t, vtkm::IdComponent i, const ComponentType& v)
    {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    //We need to convert i back to a 2d index
    const vtkm::IdComponent j = i%N;
    t[i/N][j] = v;
    }

  static inline
  ComponentType GetComponent(const Type& t, vtkm::IdComponent i)
    {
    VTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    //We need to convert i back to a 2d index
    const vtkm::IdComponent j = i%N;
    return t[i/N][j];
    }
};

} //namespace vtkmlib

#endif // vtkmlib_Portals_h
