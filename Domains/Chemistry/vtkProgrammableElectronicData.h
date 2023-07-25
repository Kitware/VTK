// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProgrammableElectronicData
 * @brief   Provides access to and storage of
 * user-generated vtkImageData that describes electrons.
 */

#ifndef vtkProgrammableElectronicData_h
#define vtkProgrammableElectronicData_h

#include "vtkAbstractElectronicData.h"
#include "vtkDomainsChemistryModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class StdVectorOfImageDataPointers;

class VTKDOMAINSCHEMISTRY_EXPORT vtkProgrammableElectronicData : public vtkAbstractElectronicData
{
public:
  static vtkProgrammableElectronicData* New();
  vtkTypeMacro(vtkProgrammableElectronicData, vtkAbstractElectronicData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the number of molecular orbitals. Setting this will resize this
   * internal array of MOs.
   */
  vtkIdType GetNumberOfMOs() override;
  virtual void SetNumberOfMOs(vtkIdType);
  ///@}

  ///@{
  /**
   * Get/Set the number of electrons in the molecule. Needed for HOMO/LUMO
   * convenience functions
   */
  vtkIdType GetNumberOfElectrons() override { return this->NumberOfElectrons; }
  vtkSetMacro(NumberOfElectrons, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/Set the vtkImageData for the requested molecular orbital.
   */
  vtkImageData* GetMO(vtkIdType orbitalNumber) override;
  void SetMO(vtkIdType orbitalNumber, vtkImageData* data);
  ///@}

  ///@{
  /**
   * Get/Set the vtkImageData for the molecule's electron density.
   */
  vtkImageData* GetElectronDensity() override { return this->ElectronDensity; }
  virtual void SetElectronDensity(vtkImageData*);
  ///@}

  ///@{
  /**
   * Set the padding around the molecule to which the cube extends. This
   * is used to determine the dataset bounds.
   */
  vtkSetMacro(Padding, double);
  ///@}

  /**
   * Deep copies the data object into this.
   */
  void DeepCopy(vtkDataObject* obj) override;

protected:
  vtkProgrammableElectronicData();
  ~vtkProgrammableElectronicData() override;

  /**
   * Electronic data set property
   */
  vtkIdType NumberOfElectrons;

  ///@{
  /**
   * Storage for the vtkImageData objects
   */
  StdVectorOfImageDataPointers* MOs;
  vtkImageData* ElectronDensity;
  ///@}

private:
  vtkProgrammableElectronicData(const vtkProgrammableElectronicData&) = delete;
  void operator=(const vtkProgrammableElectronicData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
