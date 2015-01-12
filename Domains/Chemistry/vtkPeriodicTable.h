/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPeriodicTable - Access to information about the elements.
//
// .SECTION Description
// Sourced from the Blue Obelisk Data Repository
//
// .SECTION See Also
// vtkBlueObeliskData vtkBlueObeliskDataParser

#ifndef vtkPeriodicTable_h
#define vtkPeriodicTable_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkObject.h"
#include "vtkNew.h"

class vtkBlueObeliskData;
class vtkColor3f;
class vtkLookupTable;
class vtkStdString;

class VTKDOMAINSCHEMISTRY_EXPORT vtkPeriodicTable : public vtkObject
{
public:
  vtkTypeMacro(vtkPeriodicTable, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPeriodicTable * New();

  // Description:
  // Access the static vtkBlueObeliskData object for raw access to
  // BODR data.
  vtkGetNewMacro(BlueObeliskData, vtkBlueObeliskData);

  // Description:
  // Returns the number of elements in the periodic table.
  unsigned short GetNumberOfElements();

  // Description:
  // Given an atomic number, returns the symbol associated with the
  // element
  const char * GetSymbol(const unsigned short atomicNum);

  // Description:
  // Given an atomic number, returns the name of the element
  const char * GetElementName(const unsigned short atomicNum);

  // Description:
  // Given a case-insensitive string that contains the symbol or name
  // of an element, return the corresponding atomic number.
  unsigned short GetAtomicNumber(const vtkStdString &str);
  unsigned short GetAtomicNumber(const char *str);

  // Description:
  // Given an atomic number, return the covalent radius of the atom
  float GetCovalentRadius(const unsigned short atomicNum);

  // Description:
  // Given an atomic number, returns the van der Waals radius of the
  // atom
  float GetVDWRadius(const unsigned short atomicNum);

  // Description:
  // Fill the given vtkLookupTable to map atomic numbers to the
  // familiar RGB tuples provided by the Blue Obelisk Data Repository
  void GetDefaultLUT(vtkLookupTable *);

  // Description:
  // Given an atomic number, return the familiar RGB tuple provided by
  // the Blue Obelisk Data Repository
  void GetDefaultRGBTuple(unsigned short atomicNum, float rgb[3]);

  // Description:
  // Given an atomic number, return the familiar RGB tuple provided by
  // the Blue Obelisk Data Repository
  vtkColor3f GetDefaultRGBTuple(unsigned short atomicNum);

protected:
  vtkPeriodicTable();
  ~vtkPeriodicTable();

  static vtkNew<vtkBlueObeliskData> BlueObeliskData;

private:
  vtkPeriodicTable(const vtkPeriodicTable&);   // Not implemented
  void operator=(const vtkPeriodicTable&); // Not implemented
};

#endif
