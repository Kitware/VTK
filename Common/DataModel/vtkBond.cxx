/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkBond.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBond.h"

#include "vtkAtom.h"
#include "vtkMolecule.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cassert>

//----------------------------------------------------------------------------
vtkBond::vtkBond(vtkMolecule *parent, vtkIdType id,
                 vtkIdType beginAtomId, vtkIdType endAtomId)
  : Molecule(parent), Id(id), BeginAtomId(beginAtomId), EndAtomId(endAtomId)
{
  assert(parent != 0);
  assert(id < parent->GetNumberOfBonds());
  assert(beginAtomId < parent->GetNumberOfAtoms());
  assert(endAtomId < parent->GetNumberOfAtoms());
}

//----------------------------------------------------------------------------
void vtkBond::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "Molecule: " << this->Molecule
     << " Id: " << this->Id
     << " Order: " << this->GetOrder()
     << " Length: " << this->GetLength()
     << " BeginAtomId: " << this->BeginAtomId
     << " EndAtomId: " << this->EndAtomId << endl;
}

//----------------------------------------------------------------------------
double vtkBond::GetLength() const
{
  // Reimplement here to avoid the potential cost of building the EdgeList
  // (We already know the atomIds, no need to look them up)
  vtkVector3f pos1 = this->Molecule->GetAtomPosition(this->BeginAtomId);
  vtkVector3f pos2 = this->Molecule->GetAtomPosition(this->EndAtomId);

  return (pos2 - pos1).Norm();
}

//----------------------------------------------------------------------------
vtkIdType vtkBond::GetBeginAtomId() const
{
  return this->BeginAtomId;
}

//----------------------------------------------------------------------------
vtkIdType vtkBond::GetEndAtomId() const
{
  return this->EndAtomId;
}

//----------------------------------------------------------------------------
vtkAtom vtkBond::GetBeginAtom()
{
  return this->Molecule->GetAtom(this->BeginAtomId);
}

//----------------------------------------------------------------------------
vtkAtom vtkBond::GetEndAtom()
{
  return this->Molecule->GetAtom(this->EndAtomId);
}

//----------------------------------------------------------------------------
const vtkAtom vtkBond::GetBeginAtom() const
{
  return this->Molecule->GetAtom(this->BeginAtomId);
}

//----------------------------------------------------------------------------
const vtkAtom vtkBond::GetEndAtom() const
{
  return this->Molecule->GetAtom(this->EndAtomId);
}

//----------------------------------------------------------------------------
unsigned short vtkBond::GetOrder()
{
  return this->Molecule->GetBondOrder(this->Id);
}
