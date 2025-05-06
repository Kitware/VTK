//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

#include <viskores/Bitset.h>
#include <viskores/Bounds.h>
#include <viskores/Pair.h>
#include <viskores/Range.h>

#include <viskores/TypeList.h>
#include <viskores/cont/testing/Testing.h>

#include <viskoresstd/is_trivial.h>

#include <type_traits>

namespace
{

// clang-format off
template<typename T>
void is_noexcept_movable()
{
  constexpr bool valid = std::is_nothrow_move_constructible<T>::value &&
                         std::is_nothrow_move_assignable<T>::value;

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  VISKORES_TEST_ASSERT(valid, msg);
}

// DataSet uses a map which is not nothrow constructible/assignable in the
// following implementations
template<>
void is_noexcept_movable<viskores::cont::DataSet>()
{
  using T = viskores::cont::DataSet;

  constexpr bool valid =
#if ((defined(__GNUC__) && (__GNUC__ <= 5)) || defined(_WIN32))
    std::is_move_constructible<T>::value &&
    std::is_move_assignable<T>::value;
#else
    std::is_nothrow_move_constructible<T>::value &&
    std::is_nothrow_move_assignable<T>::value;
#endif

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  VISKORES_TEST_ASSERT(valid, msg);
}

template<typename T>
void is_triv_noexcept_movable()
{
  constexpr bool valid =
#ifdef VISKORES_USE_STD_IS_TRIVIAL
                         //GCC 4.X and compilers that act like it such as Intel 17.0
                         //don't have implementations for is_trivially_*
                         std::is_trivially_move_constructible<T>::value &&
                         std::is_trivially_move_assignable<T>::value &&
#endif
                         std::is_nothrow_move_constructible<T>::value &&
                         std::is_nothrow_move_assignable<T>::value &&
                         std::is_nothrow_constructible<T, T&&>::value;

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  VISKORES_TEST_ASSERT(valid, msg);
}
// clang-format on

struct IsTrivNoExcept
{
  template <typename T>
  void operator()(T) const
  {
    is_triv_noexcept_movable<T>();
  }
};

struct IsNoExceptHandle
{
  template <typename T>
  void operator()(T) const
  {
    using HandleType = viskores::cont::ArrayHandle<T>;
    using MultiplexerType = viskores::cont::ArrayHandleMultiplexer<HandleType>;

    //verify the handle type
    is_noexcept_movable<HandleType>();
    is_noexcept_movable<MultiplexerType>();

    //verify the input portals of the handle
    is_noexcept_movable<decltype(std::declval<HandleType>().PrepareForInput(
      viskores::cont::DeviceAdapterTagSerial{}, std::declval<viskores::cont::Token&>()))>();
    is_noexcept_movable<decltype(std::declval<MultiplexerType>().PrepareForInput(
      viskores::cont::DeviceAdapterTagSerial{}, std::declval<viskores::cont::Token&>()))>();

    //verify the output portals of the handle
    is_noexcept_movable<decltype(std::declval<HandleType>().PrepareForOutput(
      2, viskores::cont::DeviceAdapterTagSerial{}, std::declval<viskores::cont::Token&>()))>();
    is_noexcept_movable<decltype(std::declval<MultiplexerType>().PrepareForOutput(
      2, viskores::cont::DeviceAdapterTagSerial{}, std::declval<viskores::cont::Token&>()))>();
  }
};

using viskoresComplexCustomTypes =
  viskores::List<viskores::Vec<viskores::Vec<float, 3>, 3>,
                 viskores::Pair<viskores::UInt64, viskores::UInt64>,
                 viskores::Bitset<viskores::UInt64>,
                 viskores::Bounds,
                 viskores::Range>;
}

//-----------------------------------------------------------------------------
void TestContDataTypesHaveMoveSemantics()
{
  //verify the Vec types are triv and noexcept
  viskores::testing::Testing::TryTypes(IsTrivNoExcept{}, viskores::TypeListVecCommon{});
  //verify that viskores::Pair, Bitset, Bounds, and Range are triv and noexcept
  viskores::testing::Testing::TryTypes(IsTrivNoExcept{}, viskoresComplexCustomTypes{});


  //verify that ArrayHandles and related portals are noexcept movable
  //allowing for efficient storage in containers such as std::vector
  viskores::testing::Testing::TryTypes(IsNoExceptHandle{}, viskores::TypeListAll{});

  viskores::testing::Testing::TryTypes(IsNoExceptHandle{}, ::viskoresComplexCustomTypes{});

  //verify the DataSet, Field, and CoordinateSystem
  //all have efficient storage in containers such as std::vector
  is_noexcept_movable<viskores::cont::DataSet>();
  is_noexcept_movable<viskores::cont::Field>();
  is_noexcept_movable<viskores::cont::CoordinateSystem>();

  //verify the CellSetStructured, and CellSetExplicit
  //have efficient storage in containers such as std::vector
  is_noexcept_movable<viskores::cont::CellSetStructured<2>>();
  is_noexcept_movable<viskores::cont::CellSetStructured<3>>();
  is_noexcept_movable<viskores::cont::CellSetExplicit<>>();
}


//-----------------------------------------------------------------------------
int UnitTestMoveConstructors(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContDataTypesHaveMoveSemantics, argc, argv);
}
