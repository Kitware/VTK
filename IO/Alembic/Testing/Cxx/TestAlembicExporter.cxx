// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAlembicExporter.h"
#include "vtkElevationFilter.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkSuperquadricSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"
#include <vtksys/SystemTools.hxx>

#include <cstdlib>

int TestAlembicExporter(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string rootname = testDirectory + "/Export";

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSuperquadricSource> torus;
  torus->ToroidalOn();
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elev->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  torus->SetCenter(1., 2., 0.);
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(torus->GetOutputPort());
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);

  // Create a simple image data with a texture
  // This will be used to test that the exporter correctly handles textures.
  // The image will be a red square.
  vtkNew<vtkImageData> image;
  image->SetDimensions(8, 8, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray* scalars =
    vtkArrayDownCast<vtkUnsignedCharArray>(image->GetPointData()->GetScalars());
  if (!scalars)
  {
    vtkLog(ERROR, "Failed to allocate scalars for image data.");
    return EXIT_FAILURE;
  }
  scalars->FillComponent(0, 255); // Set red channel to 255
  scalars->FillComponent(1, 0);   // Set green channel to 0
  scalars->FillComponent(2, 0);   // Set blue channel to 0

  vtkNew<vtkTexture> texture;
  texture->SetInputData(image);
  actor2->SetTexture(texture);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(actor2);
  renderer->ResetCamera();
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer);
  window->Render();

  std::string filename = rootname + "_full.abc";

  vtkNew<vtkAlembicExporter> exporter;
  exporter->SetRenderWindow(window);
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  auto correctSize = vtksys::SystemTools::FileLength(filename);
  if (correctSize == 0)
  {
    return EXIT_FAILURE;
  }

  // Check if the texture file for the color map was created
  std::string textureFilename = rootname + "_full_tex0.png";
  if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR, "File " << textureFilename << " for color map texture was not created.");
    return EXIT_FAILURE;
  }

  // Check if the texture file for the actor texture was created
  textureFilename = rootname + "_full_tex1.png";
  if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR, "File " << textureFilename << " for actor texture was not created.");
    return EXIT_FAILURE;
  }

  actor->VisibilityOff();
  actor2->VisibilityOff();
  filename = rootname + "_empty.abc";
  exporter->SetFileName(filename.c_str());
  exporter->Write();
  auto noDataSize = vtksys::SystemTools::FileLength(filename);
  if (noDataSize == 0)
  {
    return EXIT_FAILURE;
  }

  if (noDataSize >= correctSize)
  {
    vtkLog(ERROR,
      "File should contain data for a visible actor"
      "and not for a hidden one.");
    return EXIT_FAILURE;
  }

  textureFilename = rootname + "_empty_tex0.png";
  if (vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR,
      "File " << textureFilename
              << " for color map texture should not have been created for an empty export.");
    return EXIT_FAILURE;
  }

  actor->VisibilityOn();
  actor->SetMapper(nullptr);
  exporter->Write();
  auto size = vtksys::SystemTools::FileLength(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    vtkLog(ERROR,
      "File should not contain geometry"
      " (actor has no mapper)");
    return EXIT_FAILURE;
  }

  textureFilename = rootname + "_empty_tex0.png";
  if (vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR,
      "File " << textureFilename << " for color map texture was created but should not have been.");
    return EXIT_FAILURE;
  }

  actor->SetMapper(mapper);
  mapper->RemoveAllInputConnections(0);
  exporter->Write();
  size = vtksys::SystemTools::FileLength(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    vtkLog(ERROR,
      "File should not contain geometry"
      " (mapper has no input)");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
