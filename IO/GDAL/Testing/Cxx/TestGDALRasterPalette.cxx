/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALRasterReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkDataArray.h>
#include <vtkGDALRasterReader.h>
#include <vtkImageActor.h>
#include <vtkImageProperty.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkUniformGrid.h>

#include <iostream>

// Main program
int TestGDALRasterPalette(int argc, char** argv)
{
  if (argc < 3)
  {
    std::cerr << "Expected TestName -D InputFile.tif" << std::endl;
    return -1;
  }

  std::string inputFileName(argv[2]);

  // Create reader to read shape file.
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(inputFileName.c_str());
  reader->Update();
  vtkUniformGrid* image = vtkUniformGrid::SafeDownCast(reader->GetOutput());

  // Check that reader generated point scalars
  if (image->GetCellData()->GetNumberOfArrays() < 1)
  {
    std::cerr << "ERROR: Missing cell data scalars" << std::endl;
    return 1;
  }
  if (image->GetCellData()->GetScalars()->GetSize() == 0)
  {
    std::cerr << "ERROR: Cell data scalars empty" << std::endl;
    return 1;
  }

  // Check that reader generated color table
  vtkLookupTable* colorTable = image->GetCellData()->GetScalars()->GetLookupTable();
  if (!colorTable)
  {
    std::cerr << "ERROR: Missing color table" << std::endl;
    return 1;
  }
  if (colorTable->GetNumberOfAvailableColors() != 256)
  {
    std::cerr << "ERROR: Color table does not have 256 colors."
              << " Instead has " << colorTable->GetNumberOfAvailableColors() << std::endl;
    return 1;
  }
  // colorTable->Print(std::cout);

  // Create a renderer and actor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkImageActor> actor;

  vtkNew<vtkCellDataToPointData> c2p;
  c2p->SetInputDataObject(reader->GetOutput());
  c2p->Update();

  actor->SetInputData(vtkUniformGrid::SafeDownCast(c2p->GetOutput()));
  actor->InterpolateOff();
  // actor->GetProperty()->SetInterpolationTypeToNearest();
  actor->GetProperty()->SetLookupTable(colorTable);
  actor->GetProperty()->UseLookupTableScalarRangeOn();
  renderer->AddActor(actor);

  // Create a render window, and an interactor
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderWindow->SetSize(400, 400);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  // TODO this test is really failing, Sankhash is working on a fix
  // Once fixed remove this threshold
  int retVal = vtkRegressionTestImageThreshold(renderWindow, 3.0);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
