/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBond.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBond - convenience proxy for vtkMolecule
// .SECTION Description

#ifndef vtkBond_h
#define vtkBond_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h" // For macros, etc
#include "vtkAtom.h" // For vtkAtom

class vtkMolecule;

class VTKCOMMONDATAMODEL_EXPORT vtkBond
{
public:
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Return the Id used to identify this bond in the parent molecule.
  vtkIdType GetId() const;

  // Description:
  // Return the parent molecule of this bond.
  vtkMolecule * GetMolecule();

  // Description:
  // Get the starting / ending atom ids for this bond.
  vtkIdType GetBeginAtomId() const;
  vtkIdType GetEndAtomId() const;

  // Description:
  // Get a vtkAtom object that refers to the starting / ending atom
  // for this bond.
  vtkAtom GetBeginAtom();
  vtkAtom GetEndAtom();
  const vtkAtom GetBeginAtom() const;
  const vtkAtom GetEndAtom() const;

  // Description:
  // Get the bond order for this bond.
  unsigned short GetOrder();

  // Description:
  // Get the distance between the bonded atoms.
  //
  // @note This function is faster than vtkMolecule::GetBondLength and
  // should be used when possible.
  double GetLength() const;

protected:
  friend class vtkMolecule;

  vtkBond(vtkMolecule *parent, vtkIdType id,
          vtkIdType beginAtomId, vtkIdType endAtomId);

  vtkMolecule *Molecule;
  vtkIdType Id;
  vtkIdType BeginAtomId;
  vtkIdType EndAtomId;
};

inline vtkIdType vtkBond::GetId() const
{
  return this->Id;
}

inline vtkMolecule * vtkBond::GetMolecule()
{
  return this->Molecule;
}

#endif
// VTK-HeaderTest-Exclude: vtkBond.h
