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
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
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

// Utility function to count how many lines of a file contain a specific string
int CountStringInFile(const std::string& filePath, const std::string& searchString)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    return 0;
  }

  int count = 0;
  std::string line;
  while (std::getline(file, line))
  {
    if (line.find(searchString) != std::string::npos)
    {
      ++count;
    }
  }

  return count;
}
} // namespace

int TestUSDExporter(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    vtkLog(ERROR, "Could not determine temporary directory.");
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
  // Create shallow copies of the sphere and torus outputs
  vtkNew<vtkPolyData> sphereCopy;
  sphereCopy->ShallowCopy(vtkPolyData::SafeDownCast(sphere->GetOutputDataObject(0)));
  vtkNew<vtkPartitionedDataSet> partition0;
  partition0->SetPartition(0, sphereCopy);

  vtkNew<vtkPolyData> torusCopy;
  torusCopy->ShallowCopy(vtkPolyData::SafeDownCast(torus->GetOutputDataObject(0)));
  vtkNew<vtkPartitionedDataSet> partition1;
  partition1->SetPartition(0, torusCopy);

  vtkNew<vtkPartitionedDataSetCollection> pdc;
  pdc->SetPartitionedDataSet(0, partition0);
  pdc->SetPartitionedDataSet(1, partition1);

  // Add field data array to sphere and torus blocks
  vtkNew<vtkIntArray> sphereFieldData;
  sphereFieldData->SetName("BlockID");
  sphereFieldData->InsertNextValue(2);
  sphereCopy->GetFieldData()->AddArray(sphereFieldData);

  vtkNew<vtkIntArray> torusFieldData;
  torusFieldData->SetName("BlockID");
  torusFieldData->InsertNextValue(4);
  torusCopy->GetFieldData()->AddArray(torusFieldData);

  // Create a mapper for the composite dataset
  vtkNew<vtkCompositePolyDataMapper> compositeMapper;
  compositeMapper->ScalarVisibilityOff();
  compositeMapper->SetInputDataObject(pdc);

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
  // The vtkGroupDataSetsFilter produces an output where the two inputs
  // are at flat indices 2 and 4.
  compositeMapper->SetBlockVisibility(2, true);
  compositeMapper->SetBlockVisibility(4, false);

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
  compositeMapper->SetBlockVisibility(2, false);
  compositeMapper->SetBlockVisibility(4, true);
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
  compositeMapper->SetBlockVisibility(2, true);
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

  if (!checksPassed)
  {
    vtkLog(
      ERROR, "Test 6: one or more checks failed for the single-file multi-timestep export test.");
    return EXIT_FAILURE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 7: check that saving a scene with a composite dataset with field data
  // arrays does not crash.

  // Set both blocks visible and color by field data array. The export of the
  // field data coloring won't work (the output meshes will be solid colored)
  // because the exporter doesn't support it yet, but at least it should not crash.
  compositeMapper->SetBlockVisibility(2, true);
  compositeMapper->ScalarVisibilityOn();
  compositeMapper->SetColorModeToMapScalars();
  compositeMapper->SetScalarModeToUseFieldData();
  compositeMapper->SelectColorArray("BlockID");

  filename = rootname + "_composite3.usda";
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 7: one or more checks failed when exporting composite dataset.");
    return EXIT_FAILURE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 8: check that exporting a vtkPolyData with verts and polylines only
  // results in an exported USD file without a Mesh

  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  polydata->SetPoints(points);

  // Add vertex cells
  vtkNew<vtkCellArray> verts;
  vtkIdType vert = 0;
  verts->InsertNextCell(1, &vert);
  vert = 1;
  verts->InsertNextCell(1, &vert);
  polydata->SetVerts(verts);

  // Add polyline cells
  vtkNew<vtkCellArray> lines;
  vtkIdType line[2] = { 2, 3 };
  lines->InsertNextCell(2, line);
  polydata->SetLines(lines);

  vtkNew<vtkPolyDataMapper> vertsLinesMapper;
  vertsLinesMapper->SetInputData(polydata);
  vtkNew<vtkActor> vertsLinesActor;
  vertsLinesActor->SetMapper(vertsLinesMapper);

  renderer->RemoveAllViewProps();
  renderer->AddActor(vertsLinesActor);

  filename = rootname + "_verts_lines.usda";
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  if (!FileContainsString(filename, "Mesh"))
  {
    // Expected: no Mesh should be present
  }
  else
  {
    vtkLog(
      ERROR, "File " << filename << " should not contain a Mesh element for verts/lines only.");
    checksPassed = false;
  }

  if (enableCleanupAfterTest)
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  if (!checksPassed)
  {
    vtkLog(ERROR, "Test 8: one or more checks failed when exporting verts and polylines.");
    return EXIT_FAILURE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 9: Check that Start()/Write()/Finish() combine multiple timesteps into
  // a single USD file using time samples, rather than each Write() call
  // producing an independent file. Also exercises topology (not just point
  // positions), texture coordinates, and materials changing from frame to
  // frame, since a color-mapped scalar field can vary over an animation just
  // like geometry can.
  {
    vtkNew<vtkSphereSource> movingSphere;
    movingSphere->SetThetaResolution(4);
    movingSphere->SetPhiResolution(4);
    vtkNew<vtkElevationFilter> movingElev;
    movingElev->SetInputConnection(movingSphere->GetOutputPort());
    vtkNew<vtkPolyDataMapper> movingMapper;
    movingMapper->SetInputConnection(movingElev->GetOutputPort());
    movingMapper->SetColorModeToMapScalars();
    movingMapper->ScalarVisibilityOn();
    vtkNew<vtkActor> movingActor;
    movingActor->SetMapper(movingMapper);

    vtkNew<vtkRenderer> movingRenderer;
    movingRenderer->AddActor(movingActor);
    movingRenderer->ResetCamera();
    vtkNew<vtkRenderWindow> movingWindow;
    movingWindow->AddRenderer(movingRenderer);
    movingWindow->Render();

    vtkNew<vtkUSDExporter> timeExporter;
    timeExporter->SetRenderWindow(movingWindow);
    filename = rootname + "_timesteps.usda";
    timeExporter->SetFileName(filename.c_str());

    timeExporter->Start();
    for (int i = 0; i < 3; ++i)
    {
      movingActor->SetPosition(static_cast<double>(i), 0.0, 0.0);
      // Change topology (not just point positions) between frames.
      movingSphere->SetThetaResolution(4 + i * 4);
      movingSphere->SetPhiResolution(4 + i * 4);
      movingWindow->Render();
      timeExporter->SetTimeValue(static_cast<double>(i));
      timeExporter->Write();
    }
    timeExporter->Finish();

    auto combinedSize = vtksys::SystemTools::FileLength(filename);
    if (combinedSize == 0)
    {
      vtkLog(ERROR, "Test 9: combined timestep USD file should not be empty.");
      checksPassed = false;
    }

    if (!FileContainsString(filename, "timeSamples"))
    {
      vtkLog(ERROR, "Test 9: combined timestep USD file should contain time samples.");
      checksPassed = false;
    }

    // Topology may vary per timestep, so it should be time-sampled, not
    // written once as a static attribute.
    if (!FileContainsString(filename, "faceVertexCounts.timeSamples"))
    {
      vtkLog(ERROR, "Test 9: expected faceVertexCounts to be time-sampled.");
      checksPassed = false;
    }

    // Texture coordinates and material inputs are tied to the (potentially
    // time-varying) color-mapped scalar field, so they should be
    // time-sampled too, not written once on the first frame.
    if (!FileContainsString(filename, "primvars:st.timeSamples"))
    {
      vtkLog(ERROR, "Test 9: expected texture coordinate primvar 'st' to be time-sampled.");
      checksPassed = false;
    }
    if (!FileContainsString(filename, "diffuseColor.timeSamples") &&
      !FileContainsString(filename, "inputs:diffuseColor.timeSamples"))
    {
      vtkLog(ERROR, "Test 9: expected material diffuseColor input to be time-sampled.");
      checksPassed = false;
    }
    if (!FileContainsString(filename, "inputs:file.timeSamples"))
    {
      vtkLog(ERROR, "Test 9: expected material texture 'file' input to be time-sampled.");
      checksPassed = false;
    }

    // The color-mapped texture here is the lookup table's color ramp image,
    // which does not depend on the mesh's scalar range, only its fixed set
    // of table entries. Since the actor's lookup table does not change
    // between frames (only topology and texture coordinates do), the same
    // texture image should be reused across all 3 frames rather than
    // written out again for each one.
    std::string frame0TextureFilename = rootname + "_timesteps_tex0_frame0.png";
    if (!vtksys::SystemTools::FileExists(frame0TextureFilename.c_str(), true /* file */))
    {
      vtkLog(ERROR, "Test 9: expected texture file " << frame0TextureFilename << " to be created.");
      checksPassed = false;
    }
    else if (enableCleanupAfterTest)
    {
      vtksys::SystemTools::RemoveFile(frame0TextureFilename);
    }

    for (int i = 1; i < 3; ++i)
    {
      std::string frameTextureFilename =
        rootname + "_timesteps_tex0_frame" + std::to_string(i) + ".png";
      if (vtksys::SystemTools::FileExists(frameTextureFilename.c_str(), true /* file */))
      {
        vtkLog(ERROR,
          "Test 9: did not expect " << frameTextureFilename
                                    << " to be created since the texture did not change.");
        checksPassed = false;
        if (enableCleanupAfterTest)
        {
          vtksys::SystemTools::RemoveFile(frameTextureFilename);
        }
      }
    }

    // The reused texture file should still be referenced at every time
    // sample of the material's 'file' input.
    int frame0ReferenceCount = CountStringInFile(filename, "_timesteps_tex0_frame0.png");
    if (frame0ReferenceCount < 3)
    {
      vtkLog(ERROR,
        "Test 9: expected the reused texture file to be referenced at least 3 times (once per "
        "frame), found "
          << frame0ReferenceCount << ".");
      checksPassed = false;
    }

    // Prims themselves (as opposed to their topology) are still only
    // created once, so only one Mesh0 prim definition should appear.
    int meshDefCount = CountStringInFile(filename, "def Mesh \"Mesh0\"");
    if (meshDefCount != 1)
    {
      vtkLog(ERROR,
        "Test 9: expected exactly one Mesh0 definition in the combined timestep file, found "
          << meshDefCount << ".");
      checksPassed = false;
    }

    if (!checksPassed)
    {
      vtkLog(
        ERROR, "Test 9: one or more checks failed for the single-file multi-timestep export test.");
      return EXIT_FAILURE;
    }

    if (enableCleanupAfterTest)
    {
      vtksys::SystemTools::RemoveFile(filename);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Test 10: Check that when a texture does not change from one frame to the
  // next, the exporter reuses the previously written texture file instead of
  // writing a duplicate copy for every frame.
  {
    vtkNew<vtkSphereSource> staticTexSphere;
    staticTexSphere->SetThetaResolution(6);
    staticTexSphere->SetPhiResolution(6);

    vtkNew<vtkImageData> staticTexImage;
    staticTexImage->SetDimensions(8, 8, 1);
    staticTexImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
    vtkUnsignedCharArray* staticTexScalars =
      vtkArrayDownCast<vtkUnsignedCharArray>(staticTexImage->GetPointData()->GetScalars());
    staticTexScalars->FillComponent(0, 0);
    staticTexScalars->FillComponent(1, 255);
    staticTexScalars->FillComponent(2, 0);

    vtkNew<vtkTexture> staticTexture;
    staticTexture->SetInputData(staticTexImage);

    vtkNew<vtkPolyDataMapper> staticTexMapper;
    staticTexMapper->SetInputConnection(staticTexSphere->GetOutputPort());
    vtkNew<vtkActor> staticTexActor;
    staticTexActor->SetMapper(staticTexMapper);
    staticTexActor->SetTexture(staticTexture);

    vtkNew<vtkRenderer> staticTexRenderer;
    staticTexRenderer->AddActor(staticTexActor);
    staticTexRenderer->ResetCamera();
    vtkNew<vtkRenderWindow> staticTexWindow;
    staticTexWindow->AddRenderer(staticTexRenderer);
    staticTexWindow->Render();

    vtkNew<vtkUSDExporter> staticTexExporter;
    staticTexExporter->SetRenderWindow(staticTexWindow);
    filename = rootname + "_statictex.usda";
    staticTexExporter->SetFileName(filename.c_str());

    staticTexExporter->Start();
    for (int i = 0; i < 3; ++i)
    {
      // Move the actor each frame (so the transform is time-varying) but
      // leave the texture untouched.
      staticTexActor->SetPosition(static_cast<double>(i), 0.0, 0.0);
      staticTexWindow->Render();
      staticTexExporter->SetTimeValue(static_cast<double>(i));
      staticTexExporter->Write();
    }
    staticTexExporter->Finish();

    // Only the first frame's texture file should have been written; later
    // frames should reuse it instead of writing duplicate copies.
    std::string frame0TextureFilename = rootname + "_statictex_tex0_frame0.png";
    if (!vtksys::SystemTools::FileExists(frame0TextureFilename.c_str(), true /* file */))
    {
      vtkLog(
        ERROR, "Test 10: expected texture file " << frame0TextureFilename << " to be created.");
      checksPassed = false;
    }
    else if (enableCleanupAfterTest)
    {
      vtksys::SystemTools::RemoveFile(frame0TextureFilename);
    }

    for (int i = 1; i < 3; ++i)
    {
      std::string frameNTextureFilename =
        rootname + "_statictex_tex0_frame" + std::to_string(i) + ".png";
      if (vtksys::SystemTools::FileExists(frameNTextureFilename.c_str(), true /* file */))
      {
        vtkLog(ERROR,
          "Test 10: did not expect " << frameNTextureFilename
                                     << " to be created since the texture did not change.");
        checksPassed = false;
        if (enableCleanupAfterTest)
        {
          vtksys::SystemTools::RemoveFile(frameNTextureFilename);
        }
      }
    }

    // The material's texture file input should still be time-sampled, with
    // the unchanged first-frame texture referenced at every time code.
    if (!FileContainsString(filename, "inputs:file.timeSamples"))
    {
      vtkLog(ERROR, "Test 10: expected material texture 'file' input to be time-sampled.");
      checksPassed = false;
    }

    int frame0ReferenceCount = CountStringInFile(filename, "_statictex_tex0_frame0.png");
    if (frame0ReferenceCount < 3)
    {
      vtkLog(ERROR,
        "Test 10: expected the reused texture file to be referenced at least 3 times (once per "
        "frame), found "
          << frame0ReferenceCount << ".");
      checksPassed = false;
    }

    if (!checksPassed)
    {
      vtkLog(ERROR, "Test 10: one or more checks failed for the texture reuse test.");
      return EXIT_FAILURE;
    }

    if (enableCleanupAfterTest)
    {
      vtksys::SystemTools::RemoveFile(filename);
    }
  }

  return EXIT_SUCCESS;
}
