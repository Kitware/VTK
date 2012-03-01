/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractElectronicData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractElectronicData - Provides access to and storage of
// chemical electronic data
// .SECTION Description

#ifndef __vtkAbstractElectronicData_h
#define __vtkAbstractElectronicData_h

#include "vtkDataObject.h"

class vtkImageData;

class VTK_CHEMISTRY_EXPORT vtkAbstractElectronicData : public vtkDataObject
{
public:
  vtkTypeMacro(vtkAbstractElectronicData,vtkDataObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of molecular orbitals available.
  virtual vtkIdType GetNumberOfMOs() = 0;

  // Description:
  // Returns the number of electrons in the molecule.
  virtual unsigned int GetNumberOfElectrons() = 0;

  // Description:
  // Returns the vtkImageData for the requested molecular orbital.
  virtual vtkImageData * GetMO(vtkIdType orbitalNumber) = 0;

  // Description:
  // Returns vtkImageData for the molecule's electron density. The data
  // will be calculated when first requested, and cached for later requests.
  virtual vtkImageData * GetElectronDensity() = 0;

  // Description:
  // Returns vtkImageData for the Highest Occupied Molecular Orbital.
  vtkImageData * GetHOMO() {return this->GetMO(this->GetHOMOOrbitalNumber());}

  // Description:
  // Returns vtkImageData for the Lowest Unoccupied Molecular Orbital.
  vtkImageData * GetLUMO() {return this->GetMO(this->GetLUMOOrbitalNumber());}

  // Descripition:
  // Returns the orbital number of the Highest Occupied Molecular Orbital.
  vtkIdType GetHOMOOrbitalNumber()
  {
    return static_cast<vtkIdType>((this->GetNumberOfElectrons() / 2 ) - 1);
  }

  // Descripition:
  // Returns the orbital number of the Lowest Unoccupied Molecular Orbital.
  vtkIdType GetLUMOOrbitalNumber()
  {
    return static_cast<vtkIdType>( this->GetNumberOfElectrons() / 2 );
  }

  // Description:
  // Returns true if the given orbital number is the Highest Occupied
  // Molecular Orbital, false otherwise.
  bool IsHOMO(vtkIdType orbitalNumber)
  {
    return (orbitalNumber == this->GetHOMOOrbitalNumber());
  }

  // Description:
  // Returns true if the given orbital number is the Lowest Unoccupied
  // Molecular Orbital, false otherwise.
  bool IsLUMO(vtkIdType orbitalNumber)
  {
    return (orbitalNumber == this->GetLUMOOrbitalNumber());
  }

  // Description:
  // Deep copies the data object into this.
  virtual void DeepCopy(vtkDataObject *obj);

  // Description:
  // Get the padding between the molecule and the cube boundaries. This is
  // used to determine the dataset's bounds.
  vtkGetMacro(Padding, double);

protected:
  vtkAbstractElectronicData();
  ~vtkAbstractElectronicData();

  double Padding;

private:
  // Not implemented:
  vtkAbstractElectronicData(const vtkAbstractElectronicData&);
  void operator=(const vtkAbstractElectronicData&);
};

#endif
