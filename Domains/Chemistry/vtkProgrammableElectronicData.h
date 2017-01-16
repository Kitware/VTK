/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableElectronicData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProgrammableElectronicData
 * @brief   Provides access to and storage of
 * user-generated vtkImageData that describes electrons.
*/

#ifndef vtkProgrammableElectronicData_h
#define vtkProgrammableElectronicData_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkAbstractElectronicData.h"

class vtkImageData;

class StdVectorOfImageDataPointers;

class VTKDOMAINSCHEMISTRY_EXPORT vtkProgrammableElectronicData
    : public vtkAbstractElectronicData
{
public:
  static vtkProgrammableElectronicData *New();
  vtkTypeMacro(vtkProgrammableElectronicData,vtkAbstractElectronicData);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the number of molecular orbitals. Setting this will resize this
   * internal array of MOs.
   */
  vtkIdType GetNumberOfMOs() VTK_OVERRIDE;
  virtual void SetNumberOfMOs(vtkIdType);
  //@}

  //@{
  /**
   * Get/Set the number of electrons in the molecule. Needed for HOMO/LUMO
   * convenience functions
   */
  vtkIdType GetNumberOfElectrons() VTK_OVERRIDE
  {
    return this->NumberOfElectrons;
  }
  vtkSetMacro(NumberOfElectrons, vtkIdType);
  //@}

  //@{
  /**
   * Get/Set the vtkImageData for the requested molecular orbital.
   */
  vtkImageData * GetMO(vtkIdType orbitalNumber) VTK_OVERRIDE;
  void SetMO(vtkIdType orbitalNumber, vtkImageData *data);
  //@}

  //@{
  /**
   * Get/Set the vtkImageData for the molecule's electron density.
   */
  vtkImageData* GetElectronDensity() VTK_OVERRIDE
  {
    return this->ElectronDensity;
  }
  virtual void SetElectronDensity(vtkImageData *);
  //@}

  //@{
  /**
   * Set the padding around the molecule to which the cube extends. This
   * is used to determine the dataset bounds.
   */
  vtkSetMacro(Padding, double);
  //@}

  /**
   * Deep copies the data object into this.
   */
  void DeepCopy(vtkDataObject *obj) VTK_OVERRIDE;

protected:
  vtkProgrammableElectronicData();
  ~vtkProgrammableElectronicData() VTK_OVERRIDE;

  /**
   * Electronic data set property
   */
  vtkIdType NumberOfElectrons;

  //@{
  /**
   * Storage for the vtkImageData objects
   */
  StdVectorOfImageDataPointers *MOs;
  vtkImageData *ElectronDensity;
  //@}

private:
  vtkProgrammableElectronicData(const vtkProgrammableElectronicData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProgrammableElectronicData&) VTK_DELETE_FUNCTION;
};

#endif
