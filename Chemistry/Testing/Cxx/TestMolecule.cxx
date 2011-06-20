/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#define TEST(cond)                              \
  if (!(cond)) return false

// Example code from the molecule documentation. If this breaks,
// update the docs in vtkMolecule.h
bool MoleculeExampleCode1()
{
  vtkMolecule *mol = vtkMolecule::New();
  vtkAtom h1 = mol->AddAtom(1, 0.0, 0.0, -0.5);
  vtkAtom h2 = mol->AddAtom(1, 0.0, 0.0,  0.5);
  vtkBond b  = mol->AddBond(h1, h2, 1);

  TEST(fabs(b.GetBondLength() - 1.0) < 1e-8);
  TEST(h1.GetPositionAsVector3d().Compare(vtkVector3d(0.0, 0.0,-0.5), 1e-8));
  TEST(h2.GetPositionAsVector3d().Compare(vtkVector3d(0.0, 0.0, 0.5), 1e-8));
  TEST(h1.GetAtomicNumber() == 1);
  TEST(h2.GetAtomicNumber() == 1);

  mol->Delete();

  return true;
}

// Example code from the molecule documentation. If this breaks,
// update the docs in vtkMolecule.h
bool MoleculeExampleCode2()
{
  vtkMolecule *mol = vtkMolecule::New();

  vtkAtom h1 = mol->AddAtom();
  h1.SetAtomicNumber(1);
  h1.SetPosition(0.0, 0.0, -0.5);

  vtkAtom h2 = mol->AddAtom();
  h2.SetAtomicNumber(1);
  vtkVector3d displacement (0.0, 0.0, 1.0);
  h2.SetPosition(h1.GetPositionAsVector3d() + displacement);

  vtkBond b  = mol->AddBond(h1, h2, 1);

  TEST(fabs(b.GetBondLength() - 1.0) < 1e-8);
  TEST(h1.GetPositionAsVector3d().Compare(vtkVector3d(0.0, 0.0,-0.5), 1e-8));
  TEST(h2.GetPositionAsVector3d().Compare(vtkVector3d(0.0, 0.0, 0.5), 1e-8));
  TEST(h1.GetAtomicNumber() == 1);
  TEST(h2.GetAtomicNumber() == 1);

  mol->Delete();

  return true;
}

int TestMolecule(int argc, char *argv[])
{
  // Check that the example code given in the molecule docs compiles:
  TEST(!MoleculeExampleCode1());
  TEST(!MoleculeExampleCode2());

  return EXIT_SUCCESS;
}
