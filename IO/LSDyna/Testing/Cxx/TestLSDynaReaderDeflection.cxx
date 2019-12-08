/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLSDynaReaderDeflection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkLSDynaReader (paraview/paraview#17453)
// .SECTION Description
// Tests the vtkLSDynaReader.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkLSDynaReader.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

int TestLSDynaReaderDeflection(int argc, char* argv[])
{
  // Read file name.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/LSDyna/impact/d3plot");

  // Create the reader.
  vtkNew<vtkLSDynaReader> reader;
  reader->SetFileName(fname);
  reader->UpdateTimeStep(1.0);
  delete[] fname;

  vtkNew<vtkCompositeDataGeometryFilter> geom1;
  geom1->SetInputConnection(0, reader->GetOutputPort(0));

  // Create a mapper.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom1->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Deflection");
  mapper->CreateDefaultLookupTable();
  mapper->GetLookupTable()->SetVectorModeToMagnitude();
  mapper->GetLookupTable()->SetRange(0, 1);

  // Create the actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();
  ren->GetActiveCamera()->Pitch(-135);
  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
