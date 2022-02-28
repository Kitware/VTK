/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectionSource.h"
#include "vtkSphereSource.h"

#include "vtkRegressionTestImage.h"

int TestExtractSelection(int argc, char* argv[])
{
  vtkNew<vtkSelectionSource> selection;
  selection->SetContentType(vtkSelectionNode::INDICES);
  selection->SetFieldType(vtkSelectionNode::CELL);
  selection->AddID(-1, 2);
  selection->AddID(-1, 4);
  selection->AddID(-1, 5);
  selection->AddID(-1, 8);

  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkExtractSelection> selFilter;
  selFilter->SetInputConnection(0, sphere->GetOutputPort());
  selFilter->SetInputConnection(1, selection->GetOutputPort());

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(selFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
