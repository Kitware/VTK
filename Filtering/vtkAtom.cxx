/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkAtom.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAtom.h"

#include "vtkMolecule.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkAtom::vtkAtom(vtkMolecule *parent, vtkIdType id)
  : Molecule(parent), Id(id)
{
  assert(parent != 0);
  assert(id < parent->GetNumberOfAtoms());
}

//----------------------------------------------------------------------------
vtkAtom::~vtkAtom()
{
}

//----------------------------------------------------------------------------
void vtkAtom::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "Molecule: " << this->Molecule
     << " Id: " << this->Id
     << " Element: " << this->GetAtomicNumber()
     << " Position: " << this->GetPosition() << endl;
}

//----------------------------------------------------------------------------
unsigned short vtkAtom::GetAtomicNumber() const
{
  return this->Molecule->GetAtomAtomicNumber(this->Id);
}

//----------------------------------------------------------------------------
void vtkAtom::SetAtomicNumber(unsigned short atomicNum)
{
  this->Molecule->SetAtomAtomicNumber(this->Id, atomicNum);
}

//----------------------------------------------------------------------------
void vtkAtom::GetPosition(float pos[3]) const
{
  this->Molecule->GetAtomPosition(this->Id, pos);
}

//----------------------------------------------------------------------------
void vtkAtom::GetPosition(double pos[3]) const
{
  vtkVector3f position = this->GetPosition();
  pos[0] = position.X();
  pos[1] = position.Y();
  pos[2] = position.Z();
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(const float pos[3])
{
  this->Molecule->SetAtomPosition(this->Id, vtkVector3f(pos));
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(float x, float y, float z)
{
  this->Molecule->SetAtomPosition(this->Id, x, y, z);
}

//----------------------------------------------------------------------------
vtkVector3f vtkAtom::GetPosition() const
{
  return this->Molecule->GetAtomPosition(this->Id);
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(const vtkVector3f &pos)
{
  this->Molecule->SetAtomPosition(this->Id, pos);
}
