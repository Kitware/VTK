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
// .NAME vtkProgrammableElectronicData - Provides access to and storage of
// user-generated vtkImageData that describes electrons.

#ifndef __vtkProgrammableElectronicData_h
#define __vtkProgrammableElectronicData_h

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the number of molecular orbitals. Setting this will resize this
  // internal array of MOs.
  virtual vtkIdType GetNumberOfMOs();
  virtual void SetNumberOfMOs(vtkIdType);

  // Description:
  // Get/Set the number of electrons in the molecule. Needed for HOMO/LUMO
  // convenience functions
  vtkGetMacro(NumberOfElectrons, vtkIdType);
  vtkSetMacro(NumberOfElectrons, vtkIdType);

  // Description:
  // Get/Set the vtkImageData for the requested molecular orbital.
  virtual vtkImageData * GetMO(vtkIdType orbitalNumber);
  void SetMO(vtkIdType orbitalNumber, vtkImageData *data);

  // Description:
  // Get/Set the vtkImageData for the molecule's electron density.
  vtkGetObjectMacro(ElectronDensity, vtkImageData);
  virtual void SetElectronDensity(vtkImageData *);

  // Description:
  // Set/Get the padding around the molecule to which the cube extends. This
  // is used to determine the dataset bounds.
  vtkSetMacro(Padding, double);
  vtkGetMacro(Padding, double);

  // Description:
  // Deep copies the data object into this.
  virtual void DeepCopy(vtkDataObject *obj);

protected:
  vtkProgrammableElectronicData();
  ~vtkProgrammableElectronicData();

  // Description:
  // Electronic data set property
  vtkIdType NumberOfElectrons;

  // Description:
  // Storage for the vtkImageData objects
  StdVectorOfImageDataPointers *MOs;
  vtkImageData *ElectronDensity;

private:
  // Not implemented:
  vtkProgrammableElectronicData(const vtkProgrammableElectronicData&);
  void operator=(const vtkProgrammableElectronicData&);
};

#endif
