/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRandomHyperTreeGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"

#include "vtkNew.h"

// Mostly smoke test
int TestHyperTreeGridPreConfiguredSource(int, char*[])
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> myGenerator;

  vtkNew<vtkHyperTreeGridGeometry> geom;
  geom->SetInputConnection(myGenerator->GetOutputPort());

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_2X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_3DEPTH_2BRANCH_2X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_2DEPTH_3BRANCH_3X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_4DEPTH_3BRANCH_2X2);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_3X2X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_2DEPTH_3BRANCH_3X3X2);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);

  geom->Update();

  myGenerator->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::BALANCED);
  myGenerator->SetCustomDim(3);
  myGenerator->SetCustomFactor(2);
  myGenerator->SetCustomDepth(4);

  geom->Update();

  return EXIT_SUCCESS;
}
