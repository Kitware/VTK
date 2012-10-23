/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPeriodicTable.h"
#include "vtkNew.h"
#include "vtkColor.h"
#include "vtkStdString.h"
#include "vtkLookupTable.h"
#include "vtkMathUtilities.h"

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
  const char *nullString = "";
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
  // Test the vtkStdString variant.
  std::string symbolString("He");
  if (pTab->GetAtomicNumber(symbolString) != 2)
    {
    cout << "Failed to obtain the correct atomic number for " << symbolString
         << ": " << pTab->GetAtomicNumber(symbolString) << endl;
    ++errors;
    }

  // Check color API.
  vtkColor3f color = pTab->GetDefaultRGBTuple(6);
  vtkColor3f expectedColor(0.5, 0.5, 0.5);
  if (color[0] != expectedColor[0] || color[1] != expectedColor[1] ||
      color[2] != expectedColor[2])
    {
    ++errors;
    cout << "Expected color for carbon was incorrect: " << color[0]
         << ", " << color[1] << ", " << color[2] << endl;
    }

  float rgb[3];
  float expectedRgb[3] = { 1, 0.05, 0.05 };
  pTab->GetDefaultRGBTuple(8, rgb);
  if (rgb[0] != expectedRgb[0] || rgb[1] != expectedRgb[1] ||
      rgb[2] != expectedRgb[2])
    {
    cout << "Expected color for oxygen was incorrect: " << rgb[0]
         << ", " << rgb[1] << ", " << rgb[2] << endl;
    ++errors;
    }

  // Check atomic radius.
  float radius = pTab->GetCovalentRadius(5);
  if (!vtkMathUtilities::FuzzyCompare(radius, 0.82f, 0.01f))
    {
    cout << "Incorrect radius: " << setprecision(8) << radius << endl;
    ++errors;
    }
  radius = pTab->GetVDWRadius(56);
  if (!vtkMathUtilities::FuzzyCompare(radius, 2.7f, 0.01f))
    {
    cout << "Incorrect radius: " << setprecision(8) << radius << endl;
    ++errors;
    }

  // Obtain a lookup table for the elemental colors.
  vtkNew<vtkLookupTable> lookupTable;
  pTab->GetDefaultLUT(lookupTable.GetPointer());
  if (lookupTable->GetNumberOfColors() != 119)
    {
    cout << "Error, lookup table has " << lookupTable->GetNumberOfColors()
         << " colors, expected 119." << endl;
    ++errors;
    }

  return errors;
}
