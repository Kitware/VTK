/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkPeriodicTable.h"

#define DIE(_fmt, ...)                               \
  printf(_fmt "\n", __VA_ARGS__);                    \
  return EXIT_FAILURE

int TestPeriodicTable(int argc, char *argv[])
{
  vtkNew<vtkPeriodicTable> pTab;

  // Test that numeric strings are parsed correctly
  if (pTab->GetAtomicNumber("25") != 25)
    {
    DIE("vtkPeriodicTable::GetAtomicNumber cannot parse numeric "
        "strings properly. Given \"25\", should get 25, got %hu.",
        pTab->GetAtomicNumber("25"));
    }
  if (pTab->GetAtomicNumber("300") != 0 ||
      pTab->GetAtomicNumber("-300") != 0)
    {
    DIE("vtkPeriodicTable does not return 0 for invalid numeric strings. "
        "Given \"300\" and \"-300\", returned %hu and %hu respectively.",
        pTab->GetAtomicNumber("300"), pTab->GetAtomicNumber("-300"));
    }

  // Check that invalid strings return zero
  const char *nullString = {'\0'};
  if (pTab->GetAtomicNumber("I'm not an element.") != 0 ||
      pTab->GetAtomicNumber(0) != 0 ||
      pTab->GetAtomicNumber(nullString) != 0)
    {
    DIE("vtkPeriodicTable did not return 0 for an invalid string: %hu, %hu, %hu",
        pTab->GetAtomicNumber("I'm not an element."),
        pTab->GetAtomicNumber(0),
        pTab->GetAtomicNumber(nullString));
    }

  // Round-trip element names and symbols
  const char *symbol, *name;
  for (unsigned short i = 0; i <= pTab->GetNumberOfElements(); ++i)
    {
    name = pTab->GetElementName(i);
    symbol = pTab->GetSymbol(i);

    if (pTab->GetAtomicNumber(name) != i)
      {
      DIE("Element name failed roundtrip: Name: \"%s\" atomic number: "
          "%hu vtkPeriodicTable::GetAtomicNumber(\"%s\") returns: %hu",
          name, i, name, pTab->GetAtomicNumber(name));
      }

    if (pTab->GetAtomicNumber(symbol) != i)
      {
      DIE("Element symbol failed roundtrip: Symbol: \"%s\" atomic number: "
          "%hu vtkPeriodicTable::GetAtomicNumber(\"%s\") returns: %hu",
          symbol, i, symbol, pTab->GetAtomicNumber(symbol));
      }
    }

  // Test alternate names/symbols:
  //  - Deuterium
  if (pTab->GetAtomicNumber("D") != pTab->GetAtomicNumber("H"))
    {
    DIE("Failed to identify \"D\" as a hydrogen isotrope. "
        "Atomic number for \"D\": %hu", pTab->GetAtomicNumber("D"));
    }
  if (pTab->GetAtomicNumber("Deuterium") != pTab->GetAtomicNumber("Hydrogen"))
    {
    DIE("Failed to identify \"Deuterium\" as a hydrogen isotrope. "
        "Atomic number for \"Deuterium\": %hu",
        pTab->GetAtomicNumber("Deuterium"));
    }
  //  - Tritium
  if (pTab->GetAtomicNumber("T") != pTab->GetAtomicNumber("H"))
    {
    DIE("Failed to identify \"T\" as a hydrogen isotrope. "
        "Atomic number for \"T\": %hu", pTab->GetAtomicNumber("T"));
    }
  if (pTab->GetAtomicNumber("Tritium") != pTab->GetAtomicNumber("Hydrogen"))
    {
    DIE("Failed to identify \"Tritium\" as a hydrogen isotrope. "
        "Atomic number for \"Tritium\": %hu",
        pTab->GetAtomicNumber("Tritium"));
    }
  //  - Aluminum, Aluminium
  if (pTab->GetAtomicNumber("Aluminum") != pTab->GetAtomicNumber("Aluminium"))
    {
    DIE("\"Aluminum\" returns a different atomic number than \"Aluminium\", "
        "(%hu and %hu respectively).",
        pTab->GetAtomicNumber("Aluminum"),
        pTab->GetAtomicNumber("Aluminium"));
    }

  return EXIT_SUCCESS;
}
