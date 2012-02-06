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

int TestPeriodicTable(int , char * [])
{
  int errors = 0;
  vtkNew<vtkPeriodicTable> pTab;

  // Test that numeric strings are parsed correctly
  if (pTab->GetAtomicNumber("25") != 25)
    {
    cout << "vtkPeriodicTable::GetAtomicNumber cannot parse numeric "
         << "strings properly. Given \"25\", should get 25, got %hu."
         << pTab->GetAtomicNumber("25") << endl;
    ++errors;
    }
  if (pTab->GetAtomicNumber("300") != 0 ||
      pTab->GetAtomicNumber("-300") != 0)
    {
    cout << "vtkPeriodicTable does not return 0 for invalid numeric strings. "
         << "Given \"300\" and \"-300\", returned "
         << pTab->GetAtomicNumber("300") << " and "
         << pTab->GetAtomicNumber("-300")<< " respectively." << endl;
    ++errors;
    }

  // Check that invalid strings return zero
  const char *nullString = {'\0'};
  if (pTab->GetAtomicNumber("I'm not an element.") != 0 ||
      pTab->GetAtomicNumber(0) != 0 ||
      pTab->GetAtomicNumber(nullString) != 0)
    {
    cout << "vtkPeriodicTable did not return 0 for an invalid string: "
         << pTab->GetAtomicNumber("I'm not an element.") << ", "
         << pTab->GetAtomicNumber(0) << ", "
         << pTab->GetAtomicNumber(nullString) << endl;
    ++errors;
    }

  // Round-trip element names and symbols
  const char *symbol, *name;
  for (unsigned short i = 0; i <= pTab->GetNumberOfElements(); ++i)
    {
    name = pTab->GetElementName(i);
    symbol = pTab->GetSymbol(i);

    if (pTab->GetAtomicNumber(name) != i)
      {
      cout << "Element name failed roundtrip: Name: \""
           << name << "\" atomic number: "
           << i << "vtkPeriodicTable::GetAtomicNumber(\""
           << name << "\") returns: "
           << pTab->GetAtomicNumber(name) << endl;
      ++errors;
      }

    if (pTab->GetAtomicNumber(symbol) != i)
      {
      cout << "Element symbol failed roundtrip: Symbol: \""
           << symbol << "\" atomic number: "
           << i << " vtkPeriodicTable::GetAtomicNumber(\""
           << symbol << "\") returns: "
           << pTab->GetAtomicNumber(symbol) << endl;
      ++errors;
      }
    }

  // Test alternate names/symbols:
  //  - Deuterium
  if (pTab->GetAtomicNumber("D") != pTab->GetAtomicNumber("H"))
    {
    cout << "Failed to identify \"D\" as a hydrogen isotope. "
         << "Atomic number for \"D\": " << pTab->GetAtomicNumber("D") << endl;
    ++errors;
    }
  if (pTab->GetAtomicNumber("Deuterium") != pTab->GetAtomicNumber("Hydrogen"))
    {
    cout << "Failed to identify \"Deuterium\" as a hydrogen isotope. "
         << "Atomic number for \"Deuterium\": "
         << pTab->GetAtomicNumber("Deuterium") << endl;
    ++errors;
    }
  //  - Tritium
  if (pTab->GetAtomicNumber("T") != pTab->GetAtomicNumber("H"))
    {
    cout << "Failed to identify \"T\" as a hydrogen isotope. "
         << "Atomic number for \"T\": " << pTab->GetAtomicNumber("T") << endl;
    ++errors;
    }
  if (pTab->GetAtomicNumber("Tritium") != pTab->GetAtomicNumber("Hydrogen"))
    {
    cout << "Failed to identify \"Tritium\" as a hydrogen isotope. "
         << "Atomic number for \"Tritium\": "
         << pTab->GetAtomicNumber("Tritium") << endl;
    ++errors;
    }
  //  - Aluminum, Aluminium
  if (pTab->GetAtomicNumber("Aluminum") != pTab->GetAtomicNumber("Aluminium"))
    {
    cout << "\"Aluminum\" returns a different atomic number than \"Aluminium\", "
         << "(" << pTab->GetAtomicNumber("Aluminum") << " and "
         << pTab->GetAtomicNumber("Aluminium") << " respectively)." << endl;
    ++errors;
    }

  return errors;
}
