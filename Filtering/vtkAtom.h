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

#include "vtkObject.h" // For macros, defines, etc

class vtkMolecule;
class vtkVector3d;
class vtkVector3f;

class VTK_FILTERING_EXPORT vtkAtom
{
 public:
  ~vtkAtom();

  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Return the Id used to identify this atom in the parent molecule.
  vtkIdType GetId()
  {
    return this->Id;
  }

  // Description:
  // Return the parent molecule of this atom.
  vtkMolecule * GetMolecule()
  {
    return this->Molecule;
  }

  // Description:
  // Get/Set the atomic number of this atom
  unsigned short GetAtomicNumber();
  void SetAtomicNumber(unsigned short atomicNum);

  // Description:
  // Get/Set the position of this atom
  void GetPosition(double pos[3]);
  void SetPosition(const double pos[3]);
  void GetPosition(float pos[3]);
  void SetPosition(const float pos[3]);
  void SetPosition(float x, float y, float z);
  vtkVector3f GetPositionAsVector3f();
  void SetPosition(const vtkVector3f &pos);
  vtkVector3d GetPositionAsVector3d();
  void SetPosition(const vtkVector3d &pos);

 protected:
  friend class vtkMolecule;

  vtkAtom(vtkMolecule *parent, vtkIdType id);

  vtkMolecule *Molecule;
  vtkIdType Id;
};

#endif
