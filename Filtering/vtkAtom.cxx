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
  char buffer[80];
  double coord[3];
  this->GetPosition(coord);
  snprintf(buffer, 80, "Molecule: %p Id: %4d Ele: %3d Pos: %9.5f %9.5f %9.5f\n",
           this->Molecule, this->Id, this->GetAtomicNumber(),
           coord[0], coord[1], coord[2]);
  os << indent << buffer;
}

//----------------------------------------------------------------------------
unsigned short vtkAtom::GetAtomicNumber()
{
  return this->Molecule->GetAtomAtomicNumber(this->Id);
}

//----------------------------------------------------------------------------
void vtkAtom::SetAtomicNumber(unsigned short atomicNum)
{
  this->Molecule->SetAtomAtomicNumber(this->Id, atomicNum);
}

//----------------------------------------------------------------------------
void vtkAtom::GetPosition(double pos[3])
{
  return this->Molecule->GetAtomPosition(this->Id, pos);
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(const double pos[3])
{
  this->Molecule->SetAtomPosition(this->Id, pos);
}

//----------------------------------------------------------------------------
void vtkAtom::GetPosition(float pos[3])
{
  this->Molecule->GetAtomPosition(this->Id, pos);
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(const float pos[3])
{
  this->Molecule->SetAtomPosition(this->Id, pos);
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(float x, float y, float z)
{
  this->Molecule->SetAtomPosition(this->Id, x, y, z);
}

//----------------------------------------------------------------------------
vtkVector3f vtkAtom::GetPositionAsVector3f()
{
  return this->Molecule->GetAtomPositionAsVector3f(this->Id);
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(const vtkVector3f &pos)
{
  this->Molecule->SetAtomPosition(this->Id, pos);
}

//----------------------------------------------------------------------------
vtkVector3d vtkAtom::GetPositionAsVector3d()
{
  return this->Molecule->GetAtomPositionAsVector3d(this->Id);
}

//----------------------------------------------------------------------------
void vtkAtom::SetPosition(const vtkVector3d &pos)
{
  this->Molecule->SetAtomPosition(this->Id, pos);
}
