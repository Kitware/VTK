/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlueObeliskData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBlueObeliskData - Contains chemical data from the Blue
// Obelisk Data Repository
//
// .SECTION Description
// The Blue Obelisk Data Repository is a free, open repository of
// chemical information. This class is a container for this
// information.
//
// \note This class contains only the raw arrays parsed from the
// BODR. For more convenient access to this data, use the
// vtkPeriodicTable class.
//
// \note If you must use this class directly, consider using the
// static vtkBlueObeliskData object accessible through
// vtkPeriodicTable::GetBlueObeliskData(). This object is
// automatically populated on the first instantiation of
// vtkPeriodicTable.

#ifndef vtkBlueObeliskData_h
#define vtkBlueObeliskData_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkObject.h"
#include "vtkNew.h" // For vtkNew

class vtkAbstractArray;
class vtkFloatArray;
class vtkStringArray;
class vtkSimpleMutexLock;
class vtkUnsignedShortArray;

// Hidden STL reference: vtkstd::vector<vtkAbstractArray*>
class MyStdVectorOfVtkAbstractArrays;

class VTKDOMAINSCHEMISTRY_EXPORT vtkBlueObeliskData : public vtkObject
{
 public:
  vtkTypeMacro(vtkBlueObeliskData,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkBlueObeliskData *New();

  // Description:
  // Fill this object using an internal vtkBlueObeliskDataParser
  // instance. Check that the vtkSimpleMutexLock GetWriteMutex() is
  // locked before calling this method on a static instance in a
  // multithreaded environment.
  void Initialize();

  // Description:
  // Check if this object has been initialized yet.
  bool IsInitialized() { return this->Initialized;}

  // Description:
  // Access the mutex that protects the arrays during a call to
  // Initialize()
  vtkGetObjectMacro(WriteMutex, vtkSimpleMutexLock);

  // Description:
  // Return the number of elements for which this vtkBlueObeliskData
  // instance contains information.
  vtkGetMacro(NumberOfElements, unsigned short);

  // Description:
  // Access the raw arrays stored in this vtkBlueObeliskData.
  vtkGetNewMacro(Symbols, vtkStringArray);
  vtkGetNewMacro(LowerSymbols, vtkStringArray);
  vtkGetNewMacro(Names, vtkStringArray);
  vtkGetNewMacro(LowerNames, vtkStringArray);
  vtkGetNewMacro(PeriodicTableBlocks, vtkStringArray);
  vtkGetNewMacro(ElectronicConfigurations, vtkStringArray);
  vtkGetNewMacro(Families, vtkStringArray);

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

 protected:
  friend class vtkBlueObeliskDataParser;

  vtkBlueObeliskData();
  ~vtkBlueObeliskData();

  vtkSimpleMutexLock *WriteMutex;
  bool Initialized;

  // Description:
  // Allocate enough memory in each array for sz elements. ext is not
  // used.
  virtual int Allocate(vtkIdType sz, vtkIdType ext=1000);

  // Description:
  // Reset each array.
  virtual void Reset();

  // Description:
  // Free any unused memory in the member arrays
  virtual void Squeeze();

  unsigned short NumberOfElements;

  // Lists all arrays
  MyStdVectorOfVtkAbstractArrays *Arrays;

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
  vtkNew<vtkFloatArray> Masses; // amu
  vtkNew<vtkFloatArray> ExactMasses; // amu
  vtkNew<vtkFloatArray> IonizationEnergies; // eV
  vtkNew<vtkFloatArray> ElectronAffinities; // eV
  vtkNew<vtkFloatArray> PaulingElectronegativities; // eV
  vtkNew<vtkFloatArray> CovalentRadii; // Angstrom
  vtkNew<vtkFloatArray> VDWRadii; // Angstom
  vtkNew<vtkFloatArray> DefaultColors; // rgb 3-tuples, [0.0,1.0]
  vtkNew<vtkFloatArray> BoilingPoints; // K
  vtkNew<vtkFloatArray> MeltingPoints; // K
  vtkNew<vtkUnsignedShortArray> Periods; // Row of periodic table
  vtkNew<vtkUnsignedShortArray> Groups; // Column of periodic table

  void PrintSelfIfExists(const char *, vtkObject *, ostream&, vtkIndent);

 private:
  vtkBlueObeliskData(const vtkBlueObeliskData&); // Not implemented.
  void operator=(const vtkBlueObeliskData&); // Not implemented.
};

#endif
