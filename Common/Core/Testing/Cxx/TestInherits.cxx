// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtk::Inherits and vtk::ParentClasses.
// .SECTION Description
// Tests that superclass type-aliases can be used to gather the inheritance hierarchy.

#include "vtkDoubleArray.h"
#include "vtkInherits.h"
#include "vtkStringToken.h"

#include <set>
#include <vector>

namespace
{
struct Foo
{
};
}

int TestInherits(int, char*[])
{
  using namespace vtk::literals; // for ""_token operator.

  bool ok = true;
  std::vector<std::string> linearHierarchy;
  std::set<std::string> treeHierarchy;
  std::vector<vtkStringToken> tokens;
  vtk::Inherits<vtkDoubleArray>(linearHierarchy);
  vtk::Inherits<vtkDoubleArray>(treeHierarchy);
  vtk::Inherits<vtkDoubleArray, vtkDataArray>(tokens);

  std::cout << "as vector:\n\n";
  vtkIndent indent;
  for (const auto& typeName : linearHierarchy)
  {
    std::cout << indent << typeName << "\n";
    indent = indent.GetNextIndent();
  }

  std::cout << "\nas set:\n\n";
  for (const auto& typeName : treeHierarchy)
  {
    std::cout << typeName << "\n";
  }

  std::cout << "\nas string tokens (with early termination):\n\n";
  std::cout.fill('0');
  for (const auto& typeToken : tokens)
  {
    std::cout << "0x" << std::hex << std::setw(8) << typeToken.GetId() << std::dec << " "
              << typeToken.Data() << "\n";
  }

  std::cout << "\n";

  std::vector<std::string> expected{ "vtkDoubleArray", "vtkAOSDataArrayTemplate<double>",
    "vtkGenericDataArray<vtkAOSDataArrayTemplate<double>, double>", "vtkDataArray",
    "vtkAbstractArray", "vtkObject", "vtkObjectBase" };

  if (linearHierarchy != expected)
  {
    std::cerr << "ERROR: Inheritance hierarchy returned as vector is incorrect.\n";
    ok = false;
  }

  for (const auto& entry : expected)
  {
    if (treeHierarchy.find(entry) == treeHierarchy.end())
    {
      std::cerr << "ERROR: Inheritance hierarchy returned as set has missing entry \"" << entry
                << "\"\n";
      ok = false;
    }
  }

  if (ok && treeHierarchy.size() != 7)
  {
    std::cerr << "ERROR: Inheritance hierarchy returned as set has unexpected size.\n";
    ok = false;
  }

  if (tokens.size() == 3)
  {
    auto it = expected.begin();
    for (const auto& token : tokens)
    {
      if (*it != token)
      {
        std::cerr << "ERROR: Expected " << *it << " but got " << token.Data() << "\n";
        ok = false;
      }
      ++it;
    }
  }
  else
  {
    std::cerr << "ERROR: Token vector is wrong size (" << tokens.size() << ", expected 3)\n";
    ok = false;
  }

  if (vtk::TypeToken<vtkDoubleArray>() != vtkStringToken(expected[0]) ||
    vtk::TypeToken<vtkAOSDataArrayTemplate<double>>() != vtkStringToken(expected[1]))
  {
    std::cerr << "ERROR: vtk::TypeToken returned unexpected result.\n";
    ok = false;
  }

  // MSVC prepends "class " or "struct " to its typenames. The classes above test that
  // the former is trimmed. Test that the latter is properly trimmed.
  // This also tests that all platforms treat anonymous namespaces identically.
  if (vtk::TypeName<Foo>() != "(anonymous namespace)::Foo")
  {
    std::cerr << "ERROR: vtk::TypeName<Foo>() is \"" << vtk::TypeName<Foo>() << "\".\n";
    ok = false;
  }

  if (vtk::TypeToken<Foo>().GetId() != "(anonymous namespace)::Foo"_hash)
  {
    std::cerr << "ERROR: vtk::TypeToken<Foo>() is " << vtk::TypeToken<Foo>().GetId() << ".\n";
    ok = false;
  }

  return ok ? 0 : 1;
}
