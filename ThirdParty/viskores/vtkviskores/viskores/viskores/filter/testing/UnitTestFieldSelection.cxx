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

#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/FieldSelection.h>

namespace
{
void TestFieldSelection()
{
  {
    std::cout << "empty field selection,  everything should be false." << std::endl;
    viskores::filter::FieldSelection selection;
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
  }

  {
    std::cout << "field selection with select all,  everything should be true." << std::endl;
    viskores::filter::FieldSelection selection(viskores::filter::FieldSelection::Mode::All);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
  }

  {
    std::cout << "field selection with select none,  everything should be false." << std::endl;
    viskores::filter::FieldSelection selection(viskores::filter::FieldSelection::Mode::None);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
  }

  {
    std::cout << "field selection of one field" << std::endl;
    viskores::filter::FieldSelection selection("foo");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Cells) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
  }

  {
    std::cout << "field selection of one field/association" << std::endl;
    viskores::filter::FieldSelection selection("foo", viskores::cont::Field::Association::Points);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Cells) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (AddField)." << std::endl;
    viskores::filter::FieldSelection selection;
    selection.AddField("foo");
    selection.AddField("bar", viskores::cont::Field::Association::Cells);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (initializer list)." << std::endl;
    viskores::filter::FieldSelection selection{ "foo", "bar" };
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (std::pair initializer list)."
              << std::endl;
    using pair_type = std::pair<std::string, viskores::cont::Field::Association>;
    viskores::filter::FieldSelection selection{
      pair_type{ "foo", viskores::cont::Field::Association::Any },
      pair_type{ "bar", viskores::cont::Field::Association::Cells }
    };
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (viskores::Pair initializer list)."
              << std::endl;
    using pair_type = viskores::Pair<std::string, viskores::cont::Field::Association>;
    viskores::filter::FieldSelection selection{
      pair_type{ "foo", viskores::cont::Field::Association::Any },
      pair_type{ "bar", viskores::cont::Field::Association::Cells }
    };
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("baz") == false, "field selection failed.");

    std::cout << "  Select a field as excluded (should not change result)" << std::endl;
    selection.AddField("baz", viskores::filter::FieldSelection::Mode::Exclude);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("baz") == false, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields excluded." << std::endl;
    using pair_type = std::pair<std::string, viskores::cont::Field::Association>;
    viskores::filter::FieldSelection selection(
      { pair_type{ "foo", viskores::cont::Field::Association::Any },
        pair_type{ "bar", viskores::cont::Field::Association::Cells } },
      viskores::filter::FieldSelection::Mode::Exclude);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("baz") == true, "field selection failed.");

    std::cout << "  Select a field as included (should not change result)" << std::endl;
    selection.AddField("baz", viskores::filter::FieldSelection::Mode::Select);
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("foo", viskores::cont::Field::Association::Points) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Points) == true,
      "field selection failed.");
    VISKORES_TEST_ASSERT(
      selection.IsFieldSelected("bar", viskores::cont::Field::Association::Cells) == false,
      "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
    VISKORES_TEST_ASSERT(selection.IsFieldSelected("baz") == true, "field selection failed.");
  }
}
}

int UnitTestFieldSelection(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestFieldSelection, argc, argv);
}
