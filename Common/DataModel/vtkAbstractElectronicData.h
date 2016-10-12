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
/**
 * @class   vtkAbstractElectronicData
 * @brief   Provides access to and storage of
 * chemical electronic data
 *
*/

#ifndef vtkAbstractElectronicData_h
#define vtkAbstractElectronicData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkImageData;

class VTKCOMMONDATAMODEL_EXPORT vtkAbstractElectronicData : public vtkDataObject
{
public:
  vtkTypeMacro(vtkAbstractElectronicData,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the number of molecular orbitals available.
   */
  virtual vtkIdType GetNumberOfMOs() = 0;

  /**
   * Returns the number of electrons in the molecule.
   */
  virtual vtkIdType GetNumberOfElectrons() = 0;

  /**
   * Returns the vtkImageData for the requested molecular orbital.
   */
  virtual vtkImageData * GetMO(vtkIdType orbitalNumber) = 0;

  /**
   * Returns vtkImageData for the molecule's electron density. The data
   * will be calculated when first requested, and cached for later requests.
   */
  virtual vtkImageData * GetElectronDensity() = 0;

  /**
   * Returns vtkImageData for the Highest Occupied Molecular Orbital.
   */
  vtkImageData * GetHOMO() {return this->GetMO(this->GetHOMOOrbitalNumber());}

  /**
   * Returns vtkImageData for the Lowest Unoccupied Molecular Orbital.
   */
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

  /**
   * Returns true if the given orbital number is the Highest Occupied
   * Molecular Orbital, false otherwise.
   */
  bool IsHOMO(vtkIdType orbitalNumber)
  {
    return (orbitalNumber == this->GetHOMOOrbitalNumber());
  }

  /**
   * Returns true if the given orbital number is the Lowest Unoccupied
   * Molecular Orbital, false otherwise.
   */
  bool IsLUMO(vtkIdType orbitalNumber)
  {
    return (orbitalNumber == this->GetLUMOOrbitalNumber());
  }

  /**
   * Deep copies the data object into this.
   */
  void DeepCopy(vtkDataObject *obj) VTK_OVERRIDE;

  //@{
  /**
   * Get the padding between the molecule and the cube boundaries. This is
   * used to determine the dataset's bounds.
   */
  vtkGetMacro(Padding, double);
  //@}

protected:
  vtkAbstractElectronicData();
  ~vtkAbstractElectronicData() VTK_OVERRIDE;

  double Padding;

private:
  vtkAbstractElectronicData(const vtkAbstractElectronicData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractElectronicData&) VTK_DELETE_FUNCTION;
};

#endif
