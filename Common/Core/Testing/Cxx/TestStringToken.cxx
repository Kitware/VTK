// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkStringToken.
// .SECTION Description
// Tests build-time tokenizing strings using the vtkStringToken class.

#include "vtkDebugLeaks.h"
#include "vtkStringManager.h"
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

  if (defaultToken.GetId() != vtkStringManager::Invalid)
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

  std::cout << "Now look up strings from hash values via vtkStringManager\n";
  auto* sm = const_cast<vtkStringManager*>(vtkStringToken::GetManager());
  sm->VisitMembers([&sm](vtkStringManager::Hash hh) {
    std::cout << "  " << hh << " \"" << sm->Value(hh) << "\"\n";
    return vtkStringManager::Visit::Continue;
  });
  std::cout << "Test that missing strings resolve as empty.\n";
  auto missingString = sm->Value(missing.GetId());
  std::cout << "missing string from hash: \"" << missingString << "\" (len " << missingString.size()
            << ")\n";

  // Use the string manager to group some tokens into a named set
  // (emulating a dynamic enumeration).
  std::size_t numberVisited = 0;
  vtkStringManager::Visitor visitor = [&](vtkStringManager::Hash hh) {
    std::cout << "  " << ++numberVisited << ": " << hh << "\n";
    return vtkStringManager::Visit::Continue;
  };
  // Verify that there are no enumeration-sets to begin with.
  auto outcome = sm->VisitSets(visitor);
  if (numberVisited || outcome != vtkStringManager::Visit::Continue)
  {
    std::cerr << "ERROR: Expected an empty set of keys, found " << numberVisited << "\n";
    result = 1;
  }

  // Test adding an enumeration-set.
  vtkStringToken geomEnum("geometries");
  std::cout << "Create an enumeration set for \"geometries\" (" << geomEnum.GetId() << ")\n";
  sm->Insert(geomEnum.GetId(), "point"_hash);
  sm->Insert(geomEnum.GetId(), "curve"_hash);
  sm->Insert(geomEnum.GetId(), "surface"_hash);
  sm->Insert(geomEnum.Data(), "volume"_hash);
  outcome = sm->VisitSets(visitor);
  if (numberVisited != 1)
  {
    std::cerr << "ERROR: Expected 1 key, found " << numberVisited << "\n";
    result = 1;
  }
  numberVisited = 0;
  std::cout << "Visiting members of \"geometries\"\n";
  outcome = sm->VisitMembers(visitor, geomEnum.GetId());
  if (numberVisited != 4)
  {
    std::cerr << "ERROR: Expected 4 values, found " << numberVisited << "\n";
    result = 1;
  }

  // Test removal of a string from an enumeration-set.
  std::cout << "Remove \"volume\" from \"geometries\".\nRemaining members:\n";
  if (!sm->Remove("geometries"_hash, "volume"_hash))
  {
    std::cerr << "ERROR: Expected to remove \"volume\".\n";
    result = 1;
  }
  numberVisited = 0;
  outcome = sm->VisitMembers(visitor, geomEnum.GetId());
  if (numberVisited != 3)
  {
    std::cerr << "ERROR: Expected 4 values, found " << numberVisited << "\n";
    result = 1;
  }

  // Attempt to add an invalid member to a valid set.
  if (sm->Insert(geomEnum.GetId(), "foo"_hash))
  {
    std::cerr << "ERROR: Expected failure when adding an invalid member-hash to a set.\n";
    result = 1;
  }

  // Attempt to add a valid member to an invalid set-hash.
  if (sm->Insert("foo"_hash, "point"_hash))
  {
    std::cerr << "ERROR: Expected failure when adding a member-hash to an invalid set.\n";
    result = 1;
  }

  // Erase the set (which also erases all its members).
  std::cout << "Test erasing an enumeration set.\n";
  auto numRemoved = sm->Unmanage(geomEnum.GetId());
  if (numRemoved != 4)
  {
    std::cerr << "ERROR: Removing \"geometries\" erased " << numRemoved << " items, expected 4.\n";
    result = 1;
  }
  if (sm->Find("geometries") != vtkStringManager::Invalid)
  {
    std::cerr << "ERROR: Should have unmanaged \"geometries\".\n";
    result = 1;
  }
  if (sm->Find("point") != vtkStringManager::Invalid)
  {
    std::cerr << "ERROR: Should have unmanaged \"point\".\n";
    result = 1;
  }

  return result;
}
