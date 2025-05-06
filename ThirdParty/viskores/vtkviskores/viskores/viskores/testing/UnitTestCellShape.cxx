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

#include <viskores/CellShape.h>

#include <viskores/testing/Testing.h>

namespace
{

template <typename T>
void CheckTypeSame(T, T)
{
  std::cout << "  Success" << std::endl;
}

template <typename T1, typename T2>
void CheckTypeSame(T1, T2)
{
  VISKORES_TEST_FAIL("Got unexpected types.");
}

struct CellShapeTestFunctor
{
  template <typename ShapeTag>
  void operator()(ShapeTag) const
  {
    VISKORES_IS_CELL_SHAPE_TAG(ShapeTag);

    const viskores::IdComponent cellShapeId = ShapeTag::Id;
    std::cout << "Cell shape id: " << cellShapeId << std::endl;

    std::cout << "Check conversion between id and tag is consistent." << std::endl;
    CheckTypeSame(ShapeTag(), typename viskores::CellShapeIdToTag<cellShapeId>::Tag());

    std::cout << "Check viskoresGenericCellShapeMacro." << std::endl;
    switch (cellShapeId)
    {
      viskoresGenericCellShapeMacro(CheckTypeSame(ShapeTag(), CellShapeTag()));
      default:
        VISKORES_TEST_FAIL("Generic shape switch not working.");
    }
  }
};

void CellShapeTest()
{
  viskores::testing::Testing::TryAllCellShapes(CellShapeTestFunctor());
}

} // anonymous namespace

int UnitTestCellShape(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(CellShapeTest, argc, argv);
}
