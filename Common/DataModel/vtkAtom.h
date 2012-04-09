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
// .NAME vtkAtom - convenience proxy for vtkMolecule
// .SECTION Description

#ifndef __vtkAtom_h
#define __vtkAtom_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h" // For macros, defines, etc

class vtkMolecule;
class vtkVector3d;
class vtkVector3f;

class VTKCOMMONDATAMODEL_EXPORT vtkAtom
{
 public:
  ~vtkAtom();

  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Return the Id used to identify this atom in the parent molecule.
  vtkIdType GetId() const;

  // Description:
  // Return the parent molecule of this atom.
  vtkMolecule * GetMolecule();

  // Description:
  // Get/Set the atomic number of this atom
  unsigned short GetAtomicNumber() const;
  void SetAtomicNumber(unsigned short atomicNum);

  // Description:
  // Get/Set the position of this atom
  void GetPosition(float pos[3]) const;
  void GetPosition(double pos[3]) const;
  void SetPosition(const float pos[3]);
  void SetPosition(float x, float y, float z);
  vtkVector3f GetPosition() const;
  void SetPosition(const vtkVector3f &pos);

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
