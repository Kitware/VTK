/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiblockDisplayProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test tests setting up of display properties of individual blocks in
// composite datasets.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

int TestMultiblockDisplayProperties(int argc, char* argv[])
{
  vtkNew<vtkXMLMultiBlockDataReader> reader;
  char * fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/many_blocks/many_blocks.vtm");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkRenderWindow> renWin;

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkActor> actor;
  renderer->AddActor(actor.GetPointer());

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  actor->SetMapper(mapper.GetPointer());

  renWin->SetSize(400,400);
  renderer->GetActiveCamera()->SetViewUp(0, 0, 1);
  renderer->GetActiveCamera()->SetPosition(-1.3, 0, 1.7);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 1.6);
  renderer->SetBackground(0.1,0.2,0.4);
  renderer->ResetCamera();
  renWin->Render();

  vtkNew<vtkCompositeDataDisplayAttributes> attributes;
  mapper->SetCompositeDataDisplayAttributes(attributes.GetPointer());

  mapper->SetBlockVisibility(1, 0);
  mapper->SetBlockVisibility(23, 1);
  mapper->SetBlockVisibility(27, 0);
  mapper->SetBlockVisibility(29, 0);
  renWin->Render();

  mapper->RemoveBlockVisibility(29);
  renWin->Render();

  // Color "Group B" green.
  mapper->SetBlockColor(67, 0, 0.33, 0.0);
  renWin->Render();

  // Show "Group ACAA" and color it yellow.
  mapper->SetBlockVisibility(46, true);
  mapper->SetBlockColor(46, 1, 1, 0.5);
  renWin->Render();

  // Set opacity on "Group AC" to 0.5
  mapper->SetBlockOpacity(34, 0.5);
  renWin->Render();

  // Change solid color.
  actor->GetProperty()->SetColor(0.5, 0.1, 0.1);
  renWin->Render();

  // Remove all opacity overrides.
  mapper->RemoveBlockOpacities();
  renWin->Render();


  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  return !retVal;
}
