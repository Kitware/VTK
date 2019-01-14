/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtom.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAtom
 * @brief   convenience proxy for vtkMolecule
 *
*/

#ifndef vtkAtom_h
#define vtkAtom_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h" // For macros, defines, etc

class vtkMolecule;
class vtkVector3d;
class vtkVector3f;

class VTKCOMMONDATAMODEL_EXPORT vtkAtom
{
 public:
  void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Return the Id used to identify this atom in the parent molecule.
   */
  vtkIdType GetId() const;

  /**
   * Return the parent molecule of this atom.
   */
  vtkMolecule * GetMolecule();

  //@{
  /**
   * Get/Set the atomic number of this atom
   */
  unsigned short GetAtomicNumber() const;
  void SetAtomicNumber(unsigned short atomicNum);
  //@}

  //@{
  /**
   * Get/Set the position of this atom
   */
  void GetPosition(float pos[3]) const;
  void GetPosition(double pos[3]) const;
  void SetPosition(const float pos[3]);
  void SetPosition(float x, float y, float z);
  vtkVector3f GetPosition() const;
  void SetPosition(const vtkVector3f &pos);
  //@}

 protected:
  friend class vtkMolecule;

  vtkAtom(vtkMolecule *parent, vtkIdType id);

  vtkMolecule *Molecule;
  vtkIdType Id;
};

inline vtkIdType vtkAtom::GetId() const
{
  return this->Id;
}

inline vtkMolecule * vtkAtom::GetMolecule()
{
  return this->Molecule;
}

#endif
// VTK-HeaderTest-Exclude: vtkAtom.h
