// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkGenerateIds.h>
#include <vtkNamedColors.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTesting.h>
#include <vtkVRMLExporter.h>

int TestVRMLExporter(int argc, char* argv[])
{
  // Parse command line arguments
  std::string fieldAssociation;
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "--fieldAssociation" && i + 1 < argc)
    {
      fieldAssociation = argv[i + 1];
      break;
    }
  }

  // Create a sphere source
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(32);
  sphereSource->SetPhiResolution(32);
  sphereSource->Update();

  // Add a vtkGenerateIds filter
  vtkNew<vtkGenerateIds> generateIds;
  generateIds->SetInputConnection(sphereSource->GetOutputPort());
  generateIds->PointIdsOn();
  generateIds->CellIdsOn();
  generateIds->Update();

  // Create a mapper and color by PointIds
  vtkNew<vtkPolyDataMapper> mapper;
  vtkPolyData* polyData = vtkPolyData::SafeDownCast(generateIds->GetOutput());

  mapper->SetInputConnection(generateIds->GetOutputPort());
  if (fieldAssociation == "PointData")
  {
    mapper->SetScalarModeToUsePointFieldData();
    mapper->SelectColorArray("vtkPointIds");
    if (polyData->GetPointData()->GetArray("vtkPointIds"))
    {
      mapper->SetScalarRange(polyData->GetPointData()->GetArray("vtkPointIds")->GetRange());
    }
  }
  else if (fieldAssociation == "CellData")
  {
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SelectColorArray("vtkCellIds");
    if (polyData->GetCellData()->GetArray("vtkCellIds"))
    {
      mapper->SetScalarRange(polyData->GetCellData()->GetArray("vtkCellIds")->GetRange());
    }
  }
  else
  {
    std::cerr << "Invalid field association: " << fieldAssociation << std::endl;
    return EXIT_FAILURE;
  }

  // Create an actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Create a renderer and render window
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  // Add the actor to the renderer
  renderer->AddActor(actor);

  // Set background color
  vtkNew<vtkNamedColors> colors;
  renderer->SetBackground(colors->GetColor3d("SlateGray").GetData());

  // Render the scene
  renderWindow->Render();

  // Export the scene using vtkVRMLExporter
  std::string outputFile;
  auto vtktesting = vtkSmartPointer<vtkTesting>::New();
  vtktesting->AddArguments(argc, argv);
  outputFile = vtktesting->GetTempDirectory();
  outputFile += "/vrml-export.vrml";

  vtkNew<vtkVRMLExporter> exporter;
  exporter->SetRenderWindow(renderWindow);
  exporter->SetFileName(outputFile.c_str());
  exporter->Write();

  // Do a basic check of the output file
  if (fieldAssociation == "CellData")
  {
    // Check the output file for the text 'colorPerVertex' which should be set to false.
    std::ifstream vrmlFile(outputFile.c_str());
    if (!vrmlFile.is_open())
    {
      std::cerr << "Failed to open VRML file for reading." << std::endl;
      return EXIT_FAILURE;
    }

    std::string line;
    bool foundColorPerVertex = false;
    while (std::getline(vrmlFile, line))
    {
      if (line.find("colorPerVertex") != std::string::npos)
      {
        foundColorPerVertex = true;
        break;
      }
    }

    vrmlFile.close();

    if (!foundColorPerVertex)
    {
      std::cerr << "The string 'colorPerVertex' was not found in the VRML file." << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << "The string 'colorPerVertex' was found in the VRML file." << std::endl;
  }

  return EXIT_SUCCESS;
}
