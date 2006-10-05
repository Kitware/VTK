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

#include "vtkSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataExtractSelection.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkRegressionTestImage.h"

int TestExtractSelection( int argc, char* argv[] )
{
  vtkSelection* sel = vtkSelection::New();
  sel->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::CELL_IDS);
  
  // list of cells to be selected
  vtkIdTypeArray* arr = vtkIdTypeArray::New();
  arr->SetNumberOfTuples(4);
  arr->SetTuple1(0, 2);
  arr->SetTuple1(1, 4);
  arr->SetTuple1(2, 5);
  arr->SetTuple1(3, 8);

  sel->SetSelectionList(arr);
  arr->Delete();

  vtkSphereSource* sphere = vtkSphereSource::New();

  vtkPolyDataExtractSelection* selFilter = vtkPolyDataExtractSelection::New();
  selFilter->SetInputConnection(sphere->GetOutputPort());
  selFilter->SetSelection(sel);
  sel->Delete();

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(selFilter->GetOutputPort());

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer* ren = vtkRenderer::New();
  ren->AddActor(actor);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  sphere->Delete();
  selFilter->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
