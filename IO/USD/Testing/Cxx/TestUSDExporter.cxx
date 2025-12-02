// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkElevationFilter.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"
#include "vtkSuperquadricSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkUSDExporter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"
#include <vtksys/SystemTools.hxx>

#include <cstdlib>

// Bool to enable visual debugging
constexpr bool enableScreenshotDebugging = false;

// Bool to control whether to delete the generated files after the
// test is done with them.
constexpr bool enableCleanupAfterTest = true;

namespace
{
// Utility function to check if a file contains a specific string
bool FileContainsString(const std::string& filePath, const std::string& searchString)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    return false;
  }

  std::string line;
  while (std::getline(file, line))
  {
    if (line.find(searchString) != std::string::npos)
    {
      return true;
    }
  }

  return false;
}
} // namespace

int TestUSDExporter(int argc, char* argv[])
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

  /////////////////////////////////////////////////////////////////////////////
  // Test 1. Export a simple scene with two actors, one with a color map and
  // one with a texture, and verify that the expected texture files are created
  // and referenced in the exported USD file.
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSuperquadricSource> torus;
  torus->ToroidalOn();
  torus->SetCenter(1., 2., 0.);
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elev->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
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

  if (enableScreenshotDebugging)
  {
    // screenshot code:
    renderer->GetActiveCamera()->Azimuth(90);
    vtkNew<vtkWindowToImageFilter> w2if;
    w2if->SetInput(window);
    w2if->Update();

    vtkNew<vtkPNGWriter> writer;
    writer->SetFileName((rootname + "_screenshot.png").c_str());
    writer->SetInputConnection(w2if->GetOutputPort());
    writer->Write();
  }

  std::string filename = rootname + "_full.usda";

  vtkNew<vtkUSDExporter> exporter;
  exporter->SetRenderWindow(window);
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  auto exportFullSize = vtksys::SystemTools::FileLength(filename);
  if (exportFullSize == 0)
  {
    return EXIT_FAILURE;
  }

  bool checksPassed = true;

  // Check if the texture file for the color map was created
  std::string textureFilename = rootname + "_full_tex0.png";
  if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR, "File " << textureFilename << " for color map texture was not created.");
    checksPassed = false;
  }
  else if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(textureFilename);
  }

  // Check if the texture file is referenced from the .usda file
  if (!FileContainsString(filename, textureFilename))
  {
    vtkLog(ERROR,
      "File " << textureFilename << " for color map texture was not referenced in " << filename
              << ".");
    checksPassed = false;
  }

  // Check if the texture file for the actor texture was created
  textureFilename = rootname + "_full_tex1.png";
  if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR, "File " << textureFilename << " for actor texture was not created.");
    checksPassed = false;
  }
  else if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(textureFilename);
  }

  // Check if the texture file is referenced from the .usda file
  if (!FileContainsString(filename, textureFilename))
  {
    vtkLog(ERROR,
      "File " << textureFilename << " for color map texture was not referenced in " << filename
              << ".");
    checksPassed = false;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename.c_str());
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 1: one or more checks failed for the full export test.");
    return EXIT_FAILURE;
  }
  /////////////////////////////////////////////////////////////////////////////
  // Test 2: Check if saving a scene with no visible actors works correctly.
  // The output file should not be empty, but should not contain data for
  // any actors. Also verify that no texture files are created. Also exercise
  // saving to a .usdc file.
  actor->VisibilityOff();
  actor2->VisibilityOff();
  filename = rootname + "_empty.usdc";
  exporter->SetFileName(filename.c_str());
  exporter->Write();
  auto noDataSize = vtksys::SystemTools::FileLength(filename);
  if (noDataSize == 0)
  {
    vtkLog(ERROR, "File should not be empty even when there are no visible actors");
    checksPassed = false;
  }

  if (noDataSize >= exportFullSize)
  {
    vtkLog(ERROR,
      "File should contain data for a visible actor"
      "and not for a hidden one.");
    checksPassed = false;
  }

  textureFilename = rootname + "_empty_tex0.png";
  if (vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR,
      "File "
        << textureFilename
        << " for color map texture should not have been created because the actor is not visible.");
    checksPassed = false;
  }

  textureFilename = rootname + "_empty_tex1.png";
  if (vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR,
      "File " << textureFilename
              << " for texture should not have been created because the actor is not visible.");
    checksPassed = false;
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 2: one or more checks failed for the empty scene export test.");
    return EXIT_FAILURE;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 3: Check if saving a scene with one visible actor but no mapper works.
  // Also check that writing a binary file (.usd) works
  actor->VisibilityOn();
  actor->SetMapper(nullptr);
  filename = rootname + "_empty.usd";
  exporter->SetFileName(filename.c_str());
  exporter->Write();
  auto size = vtksys::SystemTools::FileLength(filename);
  if (size == 0)
  {
    vtkLog(ERROR, "File should not be empty even when there is no geometry");
    checksPassed = false;
  }
  if (size > noDataSize)
  {
    vtkLog(ERROR, "File should not contain geometry (actor has no mapper)");
    checksPassed = false;
  }

  textureFilename = rootname + "_empty_tex0.png";
  if (vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR,
      "File " << textureFilename << " for color map texture was created but should not have been.");
    checksPassed = false;
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 3: one or more checks failed for the no-mapper export test.");
    return EXIT_FAILURE;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 4: Check if saving a scene with one visible actor but no mapper input
  // works.
  actor->SetMapper(mapper);
  mapper->RemoveAllInputConnections(0);
  exporter->Write();
  size = vtksys::SystemTools::FileLength(filename);
  if (size == 0)
  {
    vtkLog(ERROR, "File should not be empty even when there is no geometry");
    checksPassed = false;
  }
  if (size > noDataSize)
  {
    vtkLog(ERROR, "File should not contain geometry (actor has no mapper)");
    checksPassed = false;
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 4: one or more checks failed for the no-mapper-input export test.");
    return EXIT_FAILURE;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 5: Check if saving a scene with a composite dataset works. No coloring
  // in this case. All blocks visible.
  vtkNew<vtkGroupDataSetsFilter> groupFilter;
  groupFilter->SetOutputTypeToPartitionedDataSetCollection();
  groupFilter->AddInputConnection(sphere->GetOutputPort());
  groupFilter->AddInputConnection(torus->GetOutputPort());
  groupFilter->Update();

  // Create a mapper for the composite dataset
  vtkNew<vtkCompositePolyDataMapper> compositeMapper;
  compositeMapper->ScalarVisibilityOff();
  compositeMapper->SetInputConnection(groupFilter->GetOutputPort());

  // Create an actor for the composite dataset
  vtkNew<vtkActor> compositeActor;
  compositeActor->SetMapper(compositeMapper);

  // Add the composite actor to the renderer
  renderer->RemoveAllViewProps();
  renderer->AddActor(compositeActor);
  renderer->ResetCamera();

  // Export the scene with composite dataset
  filename = rootname + "_composite0.usda";
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  auto compositeSize = vtksys::SystemTools::FileLength(filename);
  if (compositeSize == 0)
  {
    vtkLog(ERROR, "File should not be empty for composite dataset export");
    checksPassed = false;
  }

  // Check that two meshes are created in the file
  if (!(FileContainsString(filename, "def Mesh \"Mesh0\"") &&
        FileContainsString(filename, "def Mesh \"Mesh1\"")))
  {
    vtkLog(ERROR, "Composite dataset export does not contain Mesh0 and Mesh1 definitions.");
    checksPassed = false;
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 5: one or more checks failed when exporting composite dataset.");
    return EXIT_FAILURE;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 6: Check if saving a scene with a composite dataset works. No coloring
  // in this case. Only first block visible.
  auto da = vtkSmartPointer<vtkCompositeDataDisplayAttributes>::New();
  compositeMapper->SetCompositeDataDisplayAttributes(da);
  compositeMapper->SetBlockVisibility(0, true);
  compositeMapper->SetBlockVisibility(1, false);

  filename = rootname + "_composite1.usda";
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  if (FileContainsString(filename, "def Mesh \"Mesh1\""))
  {
    vtkLog(
      ERROR, "Composite dataset export contains Mesh1 definition but it should be not be present.");
    checksPassed = false;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  // Now set the second block visible and not the first
  compositeMapper->SetBlockVisibility(0, false);
  compositeMapper->SetBlockVisibility(1, true);
  exporter->Write();

  if (FileContainsString(filename, "def Mesh \"Mesh1\""))
  {
    vtkLog(ERROR, "Composite dataset export contains Mesh0 definition but it should be hidden.");
    checksPassed = false;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  // Now color by Normal X component with both blocks on
  compositeMapper->SetBlockVisibility(0, true);
  compositeMapper->ScalarVisibilityOn();
  compositeMapper->SetColorModeToMapScalars();
  compositeMapper->SetScalarModeToUsePointFieldData();
  compositeMapper->SelectColorArray("Normals");
  auto lut = compositeMapper->GetLookupTable();
  lut->SetVectorModeToComponent();
  lut->SetVectorComponent(0);
  compositeMapper->UseLookupTableScalarRangeOff();
  compositeMapper->SetScalarRange(-1.0, 1.0);

  filename = rootname + "_composite2.usda";
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  // Check if the texture file for the color map was created
  textureFilename = rootname + "_composite2_tex0.png";
  if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(
      ERROR, "File " << textureFilename << " for color map texture for block 0 was not created.");
    checksPassed = false;
  }
  else if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(textureFilename);
  }

  // Check if the texture file is referenced from the .usda file
  if (!FileContainsString(filename, textureFilename))
  {
    vtkLog(ERROR,
      "File " << textureFilename << " for color map texture for block 0 was not referenced in "
              << filename << ".");
    checksPassed = false;
  }

  // Check if the texture file for the actor texture was created
  textureFilename = rootname + "_composite2_tex1.png";
  if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true /* file */))
  {
    vtkLog(ERROR, "File " << textureFilename << " for actor texture for block 1 was not created.");
    checksPassed = false;
  }
  else if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(textureFilename);
  }

  // Check if the texture file is referenced from the .usda file
  if (!FileContainsString(filename, textureFilename))
  {
    vtkLog(ERROR,
      "File " << textureFilename << " for color map texture for block 1 was not referenced in "
              << filename << ".");
    checksPassed = false;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 6: one or more checks failed when exporting composite dataset.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
