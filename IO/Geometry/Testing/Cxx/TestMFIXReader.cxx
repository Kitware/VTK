/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMFIXReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkSmartPointer.h>
#include <vtkExecutive.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMFIXReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellData.h>

#include <vtkTestUtilities.h>
#include <vtkTestErrorObserver.h>
#include <vtkRegressionTestImage.h>

int TestMFIXReader(int argc, char *argv[])
{
  // Read file name.
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MFIXReader/BUB01.RES");

  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver1 =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver2 =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkMFIXReader> reader =
    vtkSmartPointer<vtkMFIXReader>::New();
  reader->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  reader->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver2);

  // Update without a filename should cause an error
  reader->Update();
  errorObserver1->CheckErrorMessage("No filename specified");

  reader->SetFileName(filename);
  delete [] filename;

  reader->Update();

  std::cout << "Testing reader with file: "
            << reader->GetFileName() << std::endl;
  std::cout << "There are " << reader->GetNumberOfPoints()\
            << " number of points" << std::endl;
  std::cout << "There are " << reader->GetNumberOfCells()\
            << " number of cells" << std::endl;
  std::cout << "There are " << reader->GetNumberOfCellFields()
            << " number of cell fields" << std::endl;
  reader->SetTimeStep(reader->GetNumberOfTimeSteps()/2);
  std::cout << "The timestep is  " << reader->GetTimeStep() << std::endl;
  reader->SetTimeStepRange (0, reader->GetNumberOfTimeSteps() - 1);
  std::cout << "The time step range is: "
            << reader->GetTimeStepRange()[0] << " to "
            << reader->GetTimeStepRange()[1]
            << std::endl;
  // Exercise Cell Arrays

  // 1) Default array settings
  int numberOfCellArrays = reader->GetNumberOfCellArrays();
  std::cout << "----- Default array settings" << std::endl;
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char * name = reader->GetCellArrayName(i);
    std::cout << "  Cell Array: " << i
              << " is named " << name
              << " and is "
              << (reader->GetCellArrayStatus(name) ? "Enabled" : "Disabled")
              << std::endl;
  }

  // 2) Disable one array
  std::cout << "----- Disable one array" << std::endl;
  const char * arrayName = reader->GetCellArrayName(0);
  reader->SetCellArrayStatus(arrayName, 0);
  if (reader->GetCellArrayStatus(arrayName) != 0)
  {
    std::cout << "ERROR:  Cell Array: " << "0"
              << " is named " << arrayName
              << " and should be disabled"
              << std::endl;
    return EXIT_FAILURE;
  }

  // 3) Disable all arrays
  std::cout << "----- Disable all arrays" << std::endl;
  reader->DisableAllCellArrays();
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char * name = reader->GetCellArrayName(i);
    if (reader->GetCellArrayStatus(name) != 0)
    {
      std::cout << "ERROR: " << "  Cell Array: " << i
                << " is named " << name
                << " and should be disabled"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // 4) Enable one array
  std::cout << "----- Enable one array" << std::endl;
  arrayName = reader->GetCellArrayName(0);
  reader->SetCellArrayStatus(arrayName, 1);
  if (reader->GetCellArrayStatus(arrayName) != 1)
  {
    std::cout << "ERROR:  Cell Array: " << "0"
              << " is named " << arrayName
              << " and should be disabled"
              << std::endl;
    return EXIT_FAILURE;
  }

  // 5) Enable all arrays
  std::cout << "----- Enable all arrays" << std::endl;
  reader->EnableAllCellArrays();
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char * name = reader->GetCellArrayName(i);
    if (reader->GetCellArrayStatus(name) != 1)
    {
      std::cout << "ERROR: " << "  Cell Array: " << i
                << " is named " << name
                << " and should be enabled"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  reader->Print(std::cout);

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetScalarRange(reader->GetOutput()->GetScalarRange());
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.2, .4, .6);

  renderWindow->Render();
  int retVal = vtkRegressionTestImage( renderWindow );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
