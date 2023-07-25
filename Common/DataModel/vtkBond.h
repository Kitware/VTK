// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBond
 * @brief   convenience proxy for vtkMolecule
 *
 */

#ifndef vtkBond_h
#define vtkBond_h

#include "vtkAtom.h"                  // For vtkAtom
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"                // For macros, etc

VTK_ABI_NAMESPACE_BEGIN
class vtkMolecule;

class VTKCOMMONDATAMODEL_EXPORT vtkBond
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Return the Id used to identify this bond in the parent molecule.
   */
  vtkIdType GetId() const;

  /**
   * Return the parent molecule of this bond.
   */
  vtkMolecule* GetMolecule();

  ///@{
  /**
   * Get the starting / ending atom ids for this bond.
   */
  vtkIdType GetBeginAtomId() const;
  vtkIdType GetEndAtomId() const;
  ///@}

  ///@{
  /**
   * Get a vtkAtom object that refers to the starting / ending atom
   * for this bond.
   */
  vtkAtom GetBeginAtom();
  vtkAtom GetEndAtom();
  vtkAtom GetBeginAtom() const;
  vtkAtom GetEndAtom() const;
  ///@}

  /**
   * Get the bond order for this bond.
   */
  unsigned short GetOrder();

  /**
   * Get the distance between the bonded atoms.

   * @note This function is faster than vtkMolecule::GetBondLength and
   * should be used when possible.
   */
  double GetLength() const;

protected:
  friend class vtkMolecule;

  vtkBond(vtkMolecule* parent, vtkIdType id, vtkIdType beginAtomId, vtkIdType endAtomId);

  vtkMolecule* Molecule;
  vtkIdType Id;
  vtkIdType BeginAtomId;
  vtkIdType EndAtomId;
};

inline vtkIdType vtkBond::GetId() const
{
  return this->Id;
}

inline vtkMolecule* vtkBond::GetMolecule()
{
  return this->Molecule;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkBond.h
