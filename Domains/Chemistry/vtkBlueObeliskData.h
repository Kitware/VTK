// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBlueObeliskData
 * @brief   Contains chemical data from the Blue
 * Obelisk Data Repository
 *
 *
 * The Blue Obelisk Data Repository is a free, open repository of
 * chemical information. This class is a container for this
 * information.
 *
 * \note This class contains only the raw arrays parsed from the
 * BODR. For more convenient access to this data, use the
 * vtkPeriodicTable class.
 *
 * \note If you must use this class directly, consider using the
 * static vtkBlueObeliskData object accessible through
 * vtkPeriodicTable::GetBlueObeliskData(). This object is
 * automatically populated on the first instantiation of
 * vtkPeriodicTable.
 */

#ifndef vtkBlueObeliskData_h
#define vtkBlueObeliskData_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkNew.h"                    // For vtkNew
#include "vtkObject.h"

#include <mutex> // for std::mutex

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkFloatArray;
class vtkStringArray;
class vtkUnsignedShortArray;

// Hidden STL reference: std::vector<vtkAbstractArray*>
class MyStdVectorOfVtkAbstractArrays;

class VTKDOMAINSCHEMISTRY_EXPORT vtkBlueObeliskData : public vtkObject
{
public:
  vtkTypeMacro(vtkBlueObeliskData, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkBlueObeliskData* New();

  /**
   * Fill this object using an internal vtkBlueObeliskDataParser instance.
   * Wrap this call with calls to LockWriteMutex and UnlockWriteMutex before calling
   * this method on a static instance in a multithreaded environment.
   */
  void Initialize();

  /**
   * Check if this object has been initialized yet.
   */
  bool IsInitialized() { return this->Initialized; }

  ///@{
  /**
   * Lock the mutex that protects the arrays during a call to
   * Initialize().
   */
  void LockWriteMutex();
  ///@}

  ///@{
  /**
   * Unlock the mutex that protects the arrays during a call to
   * Initialize().
   */
  void UnlockWriteMutex();
  ///@}

  ///@{
  /**
   * Return the number of elements for which this vtkBlueObeliskData
   * instance contains information.
   */
  vtkGetMacro(NumberOfElements, unsigned short);
  ///@}

  ///@{
  /**
   * Access the raw arrays stored in this vtkBlueObeliskData.
   */
  vtkGetNewMacro(Symbols, vtkStringArray);
  vtkGetNewMacro(LowerSymbols, vtkStringArray);
  vtkGetNewMacro(Names, vtkStringArray);
  vtkGetNewMacro(LowerNames, vtkStringArray);
  vtkGetNewMacro(PeriodicTableBlocks, vtkStringArray);
  vtkGetNewMacro(ElectronicConfigurations, vtkStringArray);
  vtkGetNewMacro(Families, vtkStringArray);
  ///@}

  vtkGetNewMacro(Masses, vtkFloatArray);
  vtkGetNewMacro(ExactMasses, vtkFloatArray);
  vtkGetNewMacro(IonizationEnergies, vtkFloatArray);
  vtkGetNewMacro(ElectronAffinities, vtkFloatArray);
  vtkGetNewMacro(PaulingElectronegativities, vtkFloatArray);
  vtkGetNewMacro(CovalentRadii, vtkFloatArray);
  vtkGetNewMacro(VDWRadii, vtkFloatArray);
  vtkGetNewMacro(DefaultColors, vtkFloatArray);
  vtkGetNewMacro(BoilingPoints, vtkFloatArray);
  vtkGetNewMacro(MeltingPoints, vtkFloatArray);

  vtkGetNewMacro(Periods, vtkUnsignedShortArray);
  vtkGetNewMacro(Groups, vtkUnsignedShortArray);

  /**
   * Static method to generate the data header file used by this class from the
   * BODR elements.xml. See the GenerateBlueObeliskHeader test in this module.
   */
  static bool GenerateHeaderFromXML(std::istream& xml, std::ostream& header);

protected:
  friend class vtkBlueObeliskDataParser;

  vtkBlueObeliskData();
  ~vtkBlueObeliskData() override;

  bool Initialized;

  /**
   * Allocate enough memory in each array for sz elements. ext is not
   * used.
   */
  virtual vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext = 1000);

  /**
   * Reset each array.
   */
  virtual void Reset();

  /**
   * Free any unused memory in the member arrays
   */
  virtual void Squeeze();

  unsigned short NumberOfElements;

  // Lists all arrays
  MyStdVectorOfVtkAbstractArrays* Arrays;

  // Atomic Symbols
  vtkNew<vtkStringArray> Symbols;
  vtkNew<vtkStringArray> LowerSymbols;

  // Element Names
  vtkNew<vtkStringArray> Names;
  vtkNew<vtkStringArray> LowerNames;

  // Misc Strings
  vtkNew<vtkStringArray> PeriodicTableBlocks;
  vtkNew<vtkStringArray> ElectronicConfigurations;
  vtkNew<vtkStringArray> Families; // Non-Metal, Noblegas, Metalloids, etc

  // Misc Data
  vtkNew<vtkFloatArray> Masses;                     // amu
  vtkNew<vtkFloatArray> ExactMasses;                // amu
  vtkNew<vtkFloatArray> IonizationEnergies;         // eV
  vtkNew<vtkFloatArray> ElectronAffinities;         // eV
  vtkNew<vtkFloatArray> PaulingElectronegativities; // eV
  vtkNew<vtkFloatArray> CovalentRadii;              // Angstrom
  vtkNew<vtkFloatArray> VDWRadii;                   // Angstom
  vtkNew<vtkFloatArray> DefaultColors;              // rgb 3-tuples, [0.0,1.0]
  vtkNew<vtkFloatArray> BoilingPoints;              // K
  vtkNew<vtkFloatArray> MeltingPoints;              // K
  vtkNew<vtkUnsignedShortArray> Periods;            // Row of periodic table
  vtkNew<vtkUnsignedShortArray> Groups;             // Column of periodic table

  void PrintSelfIfExists(const char*, vtkObject*, ostream&, vtkIndent);

private:
  vtkBlueObeliskData(const vtkBlueObeliskData&) = delete;
  void operator=(const vtkBlueObeliskData&) = delete;

  std::mutex NewWriteMutex;
};

VTK_ABI_NAMESPACE_END
#endif
