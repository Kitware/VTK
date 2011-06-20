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

#include <assert.h>

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
vtkBond::~vtkBond()
{
}

//----------------------------------------------------------------------------
void vtkBond::PrintSelf(ostream &os, vtkIndent indent)
{
  char buffer[80];
  double coord[3];
  snprintf(buffer, 80,
           "Parent: %p Id: %4d Order: %1d Len: %9.5f BeginAtomId: %d "
           "EndAtomId: %d\n", this->Molecule, this->Id, this->GetBondOrder(),
           this->GetBondLength(), this->BeginAtomId, this->EndAtomId);
  os << indent << buffer;

  os << indent << "Bonded Atoms:\n";
  this->Molecule->GetAtom(this->BeginAtomId).PrintSelf
    (os, indent.GetNextIndent());
  this->Molecule->GetAtom(this->EndAtomId).PrintSelf
    (os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
double vtkBond::GetBondLength()
{
  // Reimplement here to avoid the potential cost of building the EdgeList
  // (We already know the atomIds, no need to look them up)
  vtkVector3d pos1 =
      this->Molecule->GetAtomPositionAsVector3d(this->BeginAtomId);
  vtkVector3d pos2 =
      this->Molecule->GetAtomPositionAsVector3d(this->EndAtomId);

  return (pos2 - pos1).Norm();
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
unsigned short vtkBond::GetBondOrder()
{
  return this->Molecule->GetBondOrder(this->Id);
}

//----------------------------------------------------------------------------
void vtkBond::GetBeginAtomPosition(double pos[3])
{
  this->Molecule->GetAtomPosition(this->BeginAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::SetBeginAtomPosition(const double pos[3])
{
  this->Molecule->SetAtomPosition(this->BeginAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::GetEndAtomPosition(double pos[3])
{
  this->Molecule->GetAtomPosition(this->EndAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::SetEndAtomPosition(const double pos[3])
{
  this->Molecule->SetAtomPosition(this->EndAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::GetBeginAtomPosition(float pos[3])
{
  this->Molecule->GetAtomPosition(this->BeginAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::SetBeginAtomPosition(const float pos[3])
{
  this->Molecule->SetAtomPosition(this->BeginAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::GetEndAtomPosition(float pos[3])
{
  this->Molecule->GetAtomPosition(this->EndAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::SetEndAtomPosition(const float pos[3])
{
  this->Molecule->SetAtomPosition(this->EndAtomId, pos);
}

//----------------------------------------------------------------------------
void vtkBond::SetBeginAtomPosition(double x, double y, double z)
{
  this->Molecule->SetAtomPosition(this->BeginAtomId, x, y, z);
}

//----------------------------------------------------------------------------
void vtkBond::SetEndAtomPosition(double x, double y, double z)
{
  this->Molecule->SetAtomPosition(this->EndAtomId, x, y, z);
}

//----------------------------------------------------------------------------
void vtkBond::SetBeginAtomPosition(const vtkVector3f &pos)
{
  this->Molecule->SetAtomPosition(this->BeginAtomId, pos);
}

//----------------------------------------------------------------------------
vtkVector3f vtkBond::GetBeginAtomPositionAsVector3f()
{
  return this->Molecule->GetAtomPositionAsVector3f(this->BeginAtomId);
}

//----------------------------------------------------------------------------
void vtkBond::SetEndAtomPosition(const vtkVector3f &pos)
{
  this->Molecule->SetAtomPosition(this->EndAtomId, pos);
}

//----------------------------------------------------------------------------
vtkVector3f vtkBond::GetEndAtomPositionAsVector3f()
{
  return this->Molecule->GetAtomPositionAsVector3f(this->EndAtomId);
}

//----------------------------------------------------------------------------
void vtkBond::SetBeginAtomPosition(const vtkVector3d &pos)
{
  this->Molecule->SetAtomPosition(this->BeginAtomId, pos);
}

//----------------------------------------------------------------------------
vtkVector3d vtkBond::GetBeginAtomPositionAsVector3d()
{
  return this->Molecule->GetAtomPositionAsVector3d(this->BeginAtomId);
}

//----------------------------------------------------------------------------
void vtkBond::SetEndAtomPosition(const vtkVector3d &pos)
{
  this->Molecule->SetAtomPosition(this->EndAtomId, pos);
}

//----------------------------------------------------------------------------
vtkVector3d vtkBond::GetEndAtomPositionAsVector3d()
{
  return this->Molecule->GetAtomPositionAsVector3d(this->EndAtomId);
}
