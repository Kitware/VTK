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

#include <viskores/cont/UncertainCellSet.h>

#include <viskores/cont/ArrayHandleConstant.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

using NonDefaultCellSetList =
  viskores::List<viskores::cont::CellSetStructured<1>,
                 viskores::cont::CellSetExplicit<
                   viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag>>;

template <typename ExpectedCellType>
struct CheckFunctor
{
  void operator()(const ExpectedCellType&, bool& called) const { called = true; }

  template <typename UnexpectedType>
  void operator()(const UnexpectedType&, bool& called) const
  {
    VISKORES_TEST_FAIL("CastAndCall functor called with wrong type.");
    called = false;
  }
};

class DummyCellSet : public viskores::cont::CellSet
{
};

void CheckEmptyUnknownCellSet()
{
  viskores::cont::UnknownCellSet empty;

  VISKORES_TEST_ASSERT(empty.GetNumberOfCells() == 0, "UnknownCellSet should have no cells");
  VISKORES_TEST_ASSERT(empty.GetNumberOfFaces() == 0, "UnknownCellSet should have no faces");
  VISKORES_TEST_ASSERT(empty.GetNumberOfEdges() == 0, "UnknownCellSet should have no edges");
  VISKORES_TEST_ASSERT(empty.GetNumberOfPoints() == 0, "UnknownCellSet should have no points");

  empty.PrintSummary(std::cout);

  using CellSet2D = viskores::cont::CellSetStructured<2>;
  using CellSet3D = viskores::cont::CellSetStructured<3>;
  VISKORES_TEST_ASSERT(!empty.IsType<CellSet2D>(), "UnknownCellSet reports wrong type.");
  VISKORES_TEST_ASSERT(!empty.IsType<CellSet3D>(), "UnknownCellSet reports wrong type.");
  VISKORES_TEST_ASSERT(!empty.IsType<DummyCellSet>(), "UnknownCellSet reports wrong type.");

  VISKORES_TEST_ASSERT(!empty.CanConvert<CellSet2D>(), "UnknownCellSet reports wrong type.");
  VISKORES_TEST_ASSERT(!empty.CanConvert<CellSet3D>(), "UnknownCellSet reports wrong type.");
  VISKORES_TEST_ASSERT(!empty.CanConvert<DummyCellSet>(), "UnknownCellSet reports wrong type.");

  bool gotException = false;
  try
  {
    CellSet2D instance = empty.AsCellSet<CellSet2D>();
  }
  catch (viskores::cont::ErrorBadType&)
  {
    gotException = true;
  }
  VISKORES_TEST_ASSERT(gotException, "Empty UnknownCellSet should have thrown on casting");

  auto empty2 = empty.NewInstance();
  VISKORES_TEST_ASSERT(empty.GetCellSetBase() == nullptr,
                       "UnknownCellSet should contain a nullptr");
  VISKORES_TEST_ASSERT(empty2.GetCellSetBase() == nullptr,
                       "UnknownCellSet should contain a nullptr");
}

template <typename CellSetType, typename CellSetList>
void CheckUnknownCellSet(viskores::cont::UnknownCellSet unknownCellSet)
{
  VISKORES_TEST_ASSERT(unknownCellSet.CanConvert<CellSetType>());
  VISKORES_TEST_ASSERT(!unknownCellSet.CanConvert<DummyCellSet>());

  unknownCellSet.AsCellSet<CellSetType>();

  bool called = false;
  unknownCellSet.CastAndCallForTypes<CellSetList>(CheckFunctor<CellSetType>(), called);
  VISKORES_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  if (viskores::ListHas<CellSetList, VISKORES_DEFAULT_CELL_SET_LIST>::value)
  {
    called = false;
    CastAndCall(unknownCellSet, CheckFunctor<CellSetType>(), called);
    VISKORES_TEST_ASSERT(
      called, "The functor was never called (and apparently a bad value exception not thrown).");
  }

  viskores::cont::UncertainCellSet<CellSetList> uncertainCellSet(unknownCellSet);

  called = false;
  uncertainCellSet.CastAndCall(CheckFunctor<CellSetType>(), called);
  VISKORES_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  called = false;
  CastAndCall(uncertainCellSet, CheckFunctor<CellSetType>(), called);
  VISKORES_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

template <typename CellSetType>
void TryNewInstance(viskores::cont::UnknownCellSet& originalCellSet)
{
  viskores::cont::UnknownCellSet newCellSet = originalCellSet.NewInstance();

  VISKORES_TEST_ASSERT(newCellSet.IsType<CellSetType>(), "New cell set wrong type.");

  VISKORES_TEST_ASSERT(originalCellSet.GetCellSetBase() != newCellSet.GetCellSetBase(),
                       "NewInstance did not make a copy.");
}

template <typename CellSetType, typename CellSetList>
void TryCellSet(viskores::cont::UnknownCellSet& unknownCellSet)
{
  CheckUnknownCellSet<CellSetType, CellSetList>(unknownCellSet);

  CheckUnknownCellSet<CellSetType, viskores::List<CellSetType>>(unknownCellSet);

  TryNewInstance<CellSetType>(unknownCellSet);
}

template <typename CellSetType>
void TryDefaultCellSet(CellSetType cellSet)
{
  viskores::cont::UnknownCellSet unknownCellSet(cellSet);

  TryCellSet<CellSetType, VISKORES_DEFAULT_CELL_SET_LIST>(unknownCellSet);
}

template <typename CellSetType>
void TryNonDefaultCellSet(CellSetType cellSet)
{
  viskores::cont::UnknownCellSet unknownCellSet(cellSet);

  TryCellSet<CellSetType, NonDefaultCellSetList>(unknownCellSet);
}

void TestDynamicCellSet()
{
  std::cout << "Try default types with default type lists." << std::endl;
  std::cout << "*** 2D Structured Grid ******************" << std::endl;
  TryDefaultCellSet(viskores::cont::CellSetStructured<2>());
  std::cout << "*** 3D Structured Grid ******************" << std::endl;
  TryDefaultCellSet(viskores::cont::CellSetStructured<3>());
  std::cout << "*** Explicit Grid ***********************" << std::endl;
  TryDefaultCellSet(viskores::cont::CellSetExplicit<>());

  std::cout << std::endl << "Try non-default types." << std::endl;
  std::cout << "*** 1D Structured Grid ******************" << std::endl;
  TryNonDefaultCellSet(viskores::cont::CellSetStructured<1>());
  std::cout << "*** Explicit Grid Constant Shape ********" << std::endl;
  TryNonDefaultCellSet(viskores::cont::CellSetExplicit<
                       viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag>());

  std::cout << std::endl << "Try empty DynamicCellSet." << std::endl;
  CheckEmptyUnknownCellSet();
}

} // anonymous namespace

int UnitTestUnknownCellSet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDynamicCellSet, argc, argv);
}
