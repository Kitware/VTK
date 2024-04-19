// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPeriodicTable
 * @brief   Access to information about the elements.
 *
 *
 * Sourced from the Blue Obelisk Data Repository
 *
 * @sa
 * vtkBlueObeliskData vtkBlueObeliskDataParser
 */

#ifndef vtkPeriodicTable_h
#define vtkPeriodicTable_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkNew.h"                    // Needed for the static data member
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBlueObeliskData;
class vtkColor3f;
class vtkLookupTable;
class vtkStdString;

class VTKDOMAINSCHEMISTRY_EXPORT vtkPeriodicTable : public vtkObject
{
public:
  vtkTypeMacro(vtkPeriodicTable, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPeriodicTable* New();

  ///@{
  /**
   * Access the static vtkBlueObeliskData object for raw access to
   * BODR data.
   */
  vtkGetNewMacro(BlueObeliskData, vtkBlueObeliskData);
  ///@}

  /**
   * Returns the number of elements in the periodic table.
   */
  unsigned short GetNumberOfElements();

  /**
   * Given an atomic number, returns the symbol associated with the
   * element
   */
  const char* GetSymbol(unsigned short atomicNum);

  /**
   * Given an atomic number, returns the name of the element
   */
  const char* GetElementName(unsigned short atomicNum);

  ///@{
  /**
   * Given a case-insensitive string that contains the symbol or name
   * of an element, return the corresponding atomic number.
   */
  unsigned short GetAtomicNumber(const vtkStdString& str);
  unsigned short GetAtomicNumber(const char* str);
  ///@}

  /**
   * Given an atomic number, return the covalent radius of the atom
   */
  float GetCovalentRadius(unsigned short atomicNum);

  /**
   * Given an atomic number, returns the van der Waals radius of the
   * atom
   */
  float GetVDWRadius(unsigned short atomicNum);

  /**
   * Given an atomic number, returns the van der Waals radius of the
   * atom
   */
  float GetMaxVDWRadius();

  /**
   * Fill the given vtkLookupTable to map atomic numbers to the
   * familiar RGB tuples provided by the Blue Obelisk Data Repository
   */
  void GetDefaultLUT(vtkLookupTable*);

  /**
   * Given an atomic number, return the familiar RGB tuple provided by
   * the Blue Obelisk Data Repository
   */
  void GetDefaultRGBTuple(unsigned short atomicNum, float rgb[3]);

  /**
   * Given an atomic number, return the familiar RGB tuple provided by
   * the Blue Obelisk Data Repository
   */
  vtkColor3f GetDefaultRGBTuple(unsigned short atomicNum);

protected:
  vtkPeriodicTable();
  ~vtkPeriodicTable() override;

  static vtkNew<vtkBlueObeliskData> BlueObeliskData;

private:
  vtkPeriodicTable(const vtkPeriodicTable&) = delete;
  void operator=(const vtkPeriodicTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
