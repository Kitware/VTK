/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYWriterAlpha.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkPLYWriter with alpha.

#include "vtkElevationFilter.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPLYWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

int TestPLYWriterAlpha(int argc, char* argv[])
{
  // Test temporary directory
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }

  std::string testDirectory = tempDir;
  delete[] tempDir;

  const std::string outputfile = testDirectory + std::string("/") + std::string("plyAlpha.ply");

  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);

  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(sphere->GetOutputPort());
  elevation->SetLowPoint(-0.5, -0.5, -0.5);
  elevation->SetHighPoint(0.5, 0.5, 0.5);

  vtkNew<vtkLookupTable> lut;
  lut->SetTableRange(0, 1);
  lut->SetAlphaRange(0, 1.0);
  lut->Build();

  vtkNew<vtkPLYWriter> writer;
  writer->SetFileName(outputfile.c_str());
  writer->SetFileTypeToBinary();
  writer->EnableAlphaOn();
  writer->SetColorModeToDefault();
  writer->SetArrayName("Elevation");
  writer->SetLookupTable(lut);
  writer->SetInputConnection(elevation->GetOutputPort());
  writer->Write();

  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(outputfile.c_str());

  // Create a mapper.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
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
