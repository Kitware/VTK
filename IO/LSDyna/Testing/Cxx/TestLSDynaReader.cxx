/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkLSDynaReader
// .SECTION Description
// Tests the vtkLSDynaReader.

#include "vtkDebugLeaks.h"
#include "vtkLSDynaReader.h"

#include "vtkActor.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#include "vtkPNGWriter.h"
#include "vtkWindowToImageFilter.h"

#include "vtkNew.h"

int TestLSDynaReader(int argc, char* argv[])
{
  // Read file name.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/LSDyna/hemi.draw/hemi_draw.d3plot");

  // Create the reader.
  vtkNew<vtkLSDynaReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkCompositeDataGeometryFilter> geom1;
  geom1->SetInputConnection(0, reader->GetOutputPort(0));

  // Create a mapper.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom1->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();

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

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
