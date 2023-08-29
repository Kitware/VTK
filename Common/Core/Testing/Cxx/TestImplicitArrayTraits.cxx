// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImplicitArrayTraits.h"

#include "vtkSetGet.h" // for the vtkNotUsed

#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace
{

struct HasNothing
{
};

struct HasMap
{
  float map(int) const { return 0.0; }
};

struct HasMapAndMapTuple
{
  float map(int) const { return 0.0; }

  void mapTuple(int, float* tuple) const { tuple[0] = tuple[1] = tuple[2] = 0.0; }
};

struct HasMapAndMapComponent
{
  float map(int) const { return 0.0; }

  float mapComponent(int, int) const { return 0.0; }
};

struct IsClosure
{
  float operator()(int) const { return 0.0; }
};

struct IsNotDefaultConstructible
{
  IsNotDefaultConstructible(int& i) { i++; }
};

}

int TestImplicitArrayTraits(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  //--------------------------------------------------------------------------------
  int result = EXIT_SUCCESS;

  static_assert(!vtk::detail::has_map_trait<::HasNothing>::value,
    "HasNothing being attributed a has_map_trait");
  if (vtk::detail::has_map_trait<::HasNothing>::value)
  {
    std::cout << "Failed has_map_trait check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_trait<::HasNothing>::value)
  {
    std::cout << "Failed can_map_trait check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_tuple_trait<::HasNothing>::value)
  {
    std::cout << "Failed can_map_tuple_trait check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_component_trait<::HasNothing>::value)
  {
    std::cout << "Failed can_map_component_trait check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  static_assert(!vtk::detail::is_closure_trait<::HasNothing>::value,
    "HasNothing being attributed a is_closure_trait");
  if (vtk::detail::is_closure_trait<::HasNothing>::value)
  {
    std::cout << "Failed is_closure_trait check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_close_trait<::HasNothing>::value)
  {
    std::cout << "Failed can_close_trait check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::implicit_array_traits<::HasNothing>::can_read)
  {
    std::cout << "Failed can_read check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasNothing>::rtype, void>::value)
  {
    std::cout << "Failed rtype check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasNothing>::type, ::HasNothing>::value)
  {
    std::cout << "Failed type check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::implicit_array_traits<::HasNothing>::code != vtk::detail::iarrays::NONE)
  {
    std::cout << "Failed code check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  //--------------------------------------------------------------------------------
  static_assert(
    vtk::detail::has_map_trait<::HasMap>::value, "HasMap is not being attributed a has_map_trait");
  if (!vtk::detail::has_map_trait<::HasMap>::value)
  {
    std::cout << "Failed has_map_trait check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::can_map_trait<::HasMap>::value)
  {
    std::cout << "Failed can_map_trait check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_tuple_trait<::HasMap>::value)
  {
    std::cout << "Failed can_map_tuple_trait check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_component_trait<::HasMap>::value)
  {
    std::cout << "Failed can_map_component_trait check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::implicit_array_traits<::HasMap>::can_read)
  {
    std::cout << "Failed can_read check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasMap>::rtype, float>::value)
  {
    std::cout << "Failed rtype check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasMap>::type, ::HasMap>::value)
  {
    std::cout << "Failed type check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::implicit_array_traits<::HasMap>::code != vtk::detail::iarrays::MAP)
  {
    std::cout << "Failed code check on HasMap" << std::endl;
    result = EXIT_FAILURE;
  }

  //--------------------------------------------------------------------------------
  static_assert(vtk::detail::has_map_trait<::HasMapAndMapTuple>::value,
    "HasMapAndMapTuple is not being attributed a has_map_trait");
  static_assert(vtk::detail::has_map_tuple_trait<::HasMapAndMapTuple>::value,
    "HasMapAndMapTuple is not being attributed a has_map_tuple_trait");
  if (!vtk::detail::has_map_tuple_trait<::HasMapAndMapTuple>::value)
  {
    std::cout << "Failed has_map_trait check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::can_map_trait<::HasMapAndMapTuple>::value)
  {
    std::cout << "Failed can_map_trait check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::can_map_tuple_trait<::HasMapAndMapTuple>::value)
  {
    std::cout << "Failed can_map_tuple_trait check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_component_trait<::HasMapAndMapTuple>::value)
  {
    std::cout << "Failed can_map_component_trait check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::implicit_array_traits<::HasMapAndMapTuple>::can_read)
  {
    std::cout << "Failed can_read check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasMapAndMapTuple>::rtype, float>::value)
  {
    std::cout << "Failed rtype check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasMapAndMapTuple>::type,
        ::HasMapAndMapTuple>::value)
  {
    std::cout << "Failed type check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::implicit_array_traits<::HasMapAndMapTuple>::code != vtk::detail::iarrays::MAP)
  {
    std::cout << "Failed code check on HasMapAndMapTuple" << std::endl;
    result = EXIT_FAILURE;
  }

  //--------------------------------------------------------------------------------
  static_assert(vtk::detail::has_map_trait<::HasMapAndMapComponent>::value,
    "HasMapAndMapComponent is not being attributed a has_map_trait");
  static_assert(vtk::detail::has_map_component_trait<::HasMapAndMapComponent>::value,
    "HasMapAndMapComponent is not being attributed a has_map_component_trait");
  if (vtk::detail::has_map_tuple_trait<::HasMapAndMapComponent>::value)
  {
    std::cout << "Failed has_map_trait check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::can_map_trait<::HasMapAndMapComponent>::value)
  {
    std::cout << "Failed can_map_trait check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_tuple_trait<::HasMapAndMapComponent>::value)
  {
    std::cout << "Failed can_map_tuple_trait check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::can_map_component_trait<::HasMapAndMapComponent>::value)
  {
    std::cout << "Failed can_map_component_trait check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::implicit_array_traits<::HasMapAndMapComponent>::can_read)
  {
    std::cout << "Failed can_read check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasMapAndMapComponent>::rtype,
        float>::value)
  {
    std::cout << "Failed rtype check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::HasMapAndMapComponent>::type,
        ::HasMapAndMapComponent>::value)
  {
    std::cout << "Failed type check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::implicit_array_traits<::HasMapAndMapComponent>::code !=
    vtk::detail::iarrays::MAP)
  {
    std::cout << "Failed code check on HasMapAndMapComponent" << std::endl;
    result = EXIT_FAILURE;
  }

  //--------------------------------------------------------------------------------
  static_assert(vtk::detail::is_closure_trait<::IsClosure>::value,
    "IsClosure is not being attributed a is_closure_trait");
  if (!vtk::detail::is_closure_trait<::IsClosure>::value)
  {
    std::cout << "Failed is_closure_trait check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::can_close_trait<::IsClosure>::value)
  {
    std::cout << "Failed can_close_trait check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_tuple_trait<::IsClosure>::value)
  {
    std::cout << "Failed can_map_tuple_trait check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::can_map_component_trait<::IsClosure>::value)
  {
    std::cout << "Failed can_map_component_trait check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::implicit_array_traits<::IsClosure>::can_read)
  {
    std::cout << "Failed can_read check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::IsClosure>::rtype, float>::value)
  {
    std::cout << "Failed rtype check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!std::is_same<vtk::detail::implicit_array_traits<::IsClosure>::type, ::IsClosure>::value)
  {
    std::cout << "Failed type check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  if (vtk::detail::implicit_array_traits<::IsClosure>::code != vtk::detail::iarrays::CLOSURE)
  {
    std::cout << "Failed code check on IsClosure" << std::endl;
    result = EXIT_FAILURE;
  }

  //--------------------------------------------------------------------------------
  if (vtk::detail::implicit_array_traits<::IsNotDefaultConstructible>::default_constructible)
  {
    std::cout << "Failed default constructible check on IsNotDefaultConstructible" << std::endl;
    result = EXIT_FAILURE;
  }

  if (!vtk::detail::implicit_array_traits<::HasNothing>::default_constructible)
  {
    std::cout << "Failed default constructible check on HasNothing" << std::endl;
    result = EXIT_FAILURE;
  }

  return result;
}
