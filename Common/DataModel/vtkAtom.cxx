// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAtom.h"

#include "vtkMolecule.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cassert>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAtom::vtkAtom(vtkMolecule* parent, vtkIdType id)
  : Molecule(parent)
  , Id(id)
{
  assert(parent != nullptr);
  assert(id < parent->GetNumberOfAtoms());
}

//------------------------------------------------------------------------------
void vtkAtom::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Molecule: " << this->Molecule << " Id: " << this->Id
     << " Element: " << this->GetAtomicNumber() << " Position: " << this->GetPosition() << endl;
}

//------------------------------------------------------------------------------
unsigned short vtkAtom::GetAtomicNumber() const
{
  return this->Molecule->GetAtomAtomicNumber(this->Id);
}

//------------------------------------------------------------------------------
void vtkAtom::SetAtomicNumber(unsigned short atomicNum)
{
  this->Molecule->SetAtomAtomicNumber(this->Id, atomicNum);
}

//------------------------------------------------------------------------------
void vtkAtom::GetPosition(float pos[3]) const
{
  this->Molecule->GetAtomPosition(this->Id, pos);
}

//------------------------------------------------------------------------------
void vtkAtom::GetPosition(double pos[3]) const
{
  this->Molecule->GetAtomPosition(this->Id, pos);
}

//------------------------------------------------------------------------------
void vtkAtom::SetPosition(const float pos[3])
{
  this->Molecule->SetAtomPosition(this->Id, vtkVector3f(pos));
}

//------------------------------------------------------------------------------
void vtkAtom::SetPosition(float x, float y, float z)
{
  this->Molecule->SetAtomPosition(this->Id, x, y, z);
}

//------------------------------------------------------------------------------
vtkVector3f vtkAtom::GetPosition() const
{
  return this->Molecule->GetAtomPosition(this->Id);
}

//------------------------------------------------------------------------------
void vtkAtom::SetPosition(const vtkVector3f& pos)
{
  this->Molecule->SetAtomPosition(this->Id, pos);
}
VTK_ABI_NAMESPACE_END
