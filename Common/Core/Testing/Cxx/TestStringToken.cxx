// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkStringToken.
// .SECTION Description
// Tests build-time tokenizing strings using the vtkStringToken class.

#include "vtkDebugLeaks.h"
#include "vtkStringToken.h"

#include <string>
#include <vector>

int TestStringToken(int, char*[])
{
  using namespace vtk::literals; // for ""_token().
  int result = 0;

  vtkStringToken defaultToken; // default constructor should be Invalid token.
  vtkStringToken eid("");
  auto missing = "not present in manager because token constructed from hash, not string."_token;
  vtkStringToken fooTheBar1("foo the bar");
  auto fooTheBar2 = "foo the bar"_token;

  std::cout << "default token is " << defaultToken.GetId() << "\n";
  std::cout << "empty string is " << eid.GetId() << "\n";
  std::cout << "missing is " << missing.GetId() << "\n";
  std::cout << "foo the bar is " << fooTheBar1.GetId() << " == " << fooTheBar2.GetId() << "\n";

  if (defaultToken.IsValid())
  {
    std::cerr << "ERROR: Default token constructor should be initialized to Invalid token\n";
  }

  std::vector<std::pair<std::string, vtkStringToken::Hash>> tests{ { { "", 2166136261u },
    { "a", 3826002220u }, { "b", 3876335077u }, { "cell", 1759288501u }, { "curve", 2570585620u },
    { "edge", 1459017788u }, { "face", 292255708u }, { "point", 414084241u },
    { "surface", 425316092u }, { "vertex", 2488493991u }, { "volume", 786459023u } } };
  for (const auto& test : tests)
  {
    vtkStringToken xx(test.first);
    std::cout << "  " << xx.GetId() << " \"" << test.first << "\"\n";
    if (test.second != xx.GetId())
    {
      std::cerr << "    ERROR: Expected " << test.second << " got " << xx.GetId() << "\n";
      result = 1;
    }
  }

  // Group some tokens into a named set (emulating a dynamic enumeration).

  // I. Verify that there are no enumeration-sets to begin with.
  auto groups = vtkStringToken::AllGroups();
  if (!groups.empty())
  {
    std::cerr << "ERROR: Expected an empty set of keys, found " << groups.size() << "\n";
    result = 1;
  }

  // Test adding an enumeration-set.
  vtkStringToken geomEnum("geometries");
  std::cout << "Create an enumeration set for \"geometries\" (" << geomEnum.GetId() << ")\n";
  bool added = true;
  added &= geomEnum.AddChild("point"_hash);
  added &= geomEnum.AddChild("curve"_hash);
  added &= geomEnum.AddChild("surface"_hash);
  added &= geomEnum.AddChild("volume"_hash);
  if (!added)
  {
    std::cerr << "ERROR: Failed to add at least 1 child.\n";
    result = 1;
  }
  groups = vtkStringToken::AllGroups();
  if (groups.size() != 1)
  {
    std::cerr << "ERROR: Expected 1 key, found " << groups.size() << "\n";
    result = 1;
  }
  std::cout << "Members of \"geometries\"\n";
  auto members = geomEnum.Children();
  for (const auto& member : members)
  {
    std::cout << "  " << member.Data() << "\n";
  }
  if (members.size() != 4)
  {
    std::cerr << "ERROR: Expected 4 values, found " << members.size() << "\n";
    result = 1;
  }

  // Test removal of a string from an enumeration-set.
  std::cout << "Remove \"volume\" from \"geometries\".\nRemaining members:\n";
  if (!geomEnum.RemoveChild("volume"_hash))
  {
    std::cerr << "ERROR: Expected to remove \"volume\".\n";
    result = 1;
  }
  members = geomEnum.Children();
  for (const auto& member : members)
  {
    std::cout << "  " << member.Data() << "\n";
  }
  if (members.size() != 3)
  {
    std::cerr << "ERROR: Expected 3 values, found " << members.size() << "\n";
    result = 1;
  }

  // Attempt to add an invalid member to a valid set.
  if (geomEnum.AddChild("foo"_hash))
  {
    std::cerr << "ERROR: Expected failure when adding an invalid member-hash to a set.\n";
    result = 1;
  }

  // Attempt to add a valid member to an invalid set-hash.
  vtkStringToken invalid;
  if (invalid.AddChild("point"_token))
  {
    std::cerr << "ERROR: Expected failure when adding a member-hash to an invalid set.\n";
    result = 1;
  }

  std::cout << "Add one more group\n";
  vtkStringToken car("car");
  car.AddChild("body");
  car.AddChild("wheels");
  car.AddChild("windows");
  car.AddChild("motor");
  groups = vtkStringToken::AllGroups();
  for (const auto& group : groups)
  {
    std::cout << "  " << group.Data() << "\n";
  }
  if (groups.size() != 2)
  {
    std::cerr << "ERROR: Expected 2 groups, got " << groups.size() << ".\n";
    result = 1;
  }

  return result;
}
