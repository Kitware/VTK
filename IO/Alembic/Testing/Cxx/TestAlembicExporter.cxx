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
#include <vtksys/Glob.hxx>
#include <vtksys/SystemTools.hxx>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <cstdlib>

#include <iostream>

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

  std::string rootname = testDirectory + "/TestAlembicExporter";

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

  /////////////////////////////////////////////////////////////////////////////
  // Test: Check that Start()/Write()/Finish() combine multiple timesteps into
  // a single Alembic file using time samples, rather than each Write() call
  // producing a potentially independent file. Exercises topology (not just point
  // positions), and texture coordinates changing from frame to
  // frame, and confirms a texture image is only re-written when its content
  // actually changed since the previous frame that wrote one.
  {
    bool checksPassed = true;

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

    // A second actor with an explicit texture that we mutate on the last
    // frame only, so we can confirm a new PNG is written only when the
    // texture content actually changes.
    vtkNew<vtkImageData> texImage;
    texImage->SetDimensions(4, 4, 1);
    texImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
    vtkUnsignedCharArray* texScalars =
      vtkArrayDownCast<vtkUnsignedCharArray>(texImage->GetPointData()->GetScalars());
    texScalars->FillComponent(0, 255);
    texScalars->FillComponent(1, 0);
    texScalars->FillComponent(2, 0);
    vtkNew<vtkTexture> movingTexture;
    movingTexture->SetInputData(texImage);

    vtkNew<vtkSuperquadricSource> texturedQuadric;
    vtkNew<vtkPolyDataMapper> texturedMapper;
    texturedMapper->SetInputConnection(texturedQuadric->GetOutputPort());
    texturedMapper->ScalarVisibilityOff();
    vtkNew<vtkActor> texturedActor;
    texturedActor->SetMapper(texturedMapper);
    texturedActor->SetTexture(movingTexture);

    vtkNew<vtkRenderer> movingRenderer;
    movingRenderer->AddActor(movingActor);
    movingRenderer->AddActor(texturedActor);
    movingRenderer->ResetCamera();
    vtkNew<vtkRenderWindow> movingWindow;
    movingWindow->AddRenderer(movingRenderer);
    movingWindow->Render();

    vtkNew<vtkAlembicExporter> timeExporter;
    timeExporter->SetRenderWindow(movingWindow);
    std::string timestepsFilename = rootname + "_timesteps.abc";
    timeExporter->SetFileName(timestepsFilename.c_str());

    timeExporter->Start();
    for (int i = 0; i < 3; ++i)
    {
      movingActor->SetPosition(static_cast<double>(i), 0.0, 0.0);
      // Change topology (not just point positions) between frames.
      movingSphere->SetThetaResolution(4 + i * 4);
      movingSphere->SetPhiResolution(4 + i * 4);
      if (i == 2)
      {
        // Change the second actor's texture content only on the last frame.
        texScalars->FillComponent(0, 0);
        texScalars->FillComponent(2, 255);
        texImage->Modified();
      }
      movingWindow->Render();
      timeExporter->SetTimeValue(static_cast<double>(i));
      timeExporter->Write();
    }
    timeExporter->Finish();

    auto combinedSize = vtksys::SystemTools::FileLength(timestepsFilename);
    if (combinedSize == 0)
    {
      vtkLog(ERROR, "Test: combined timestep Alembic file should not be empty.");
      checksPassed = false;
    }

    // The color-mapped sphere's LUT texture doesn't change across frames
    // (elevation range is unaffected by the position/topology changes made
    // above), so only frame 0's texture image should have been written.
    std::string tex0Frame0 = rootname + "_timesteps_tex0_frame0.png";
    std::string tex0Frame1 = rootname + "_timesteps_tex0_frame1.png";
    std::string tex0Frame2 = rootname + "_timesteps_tex0_frame2.png";
    if (!vtksys::SystemTools::FileExists(tex0Frame0.c_str(), true /* file */))
    {
      vtkLog(ERROR, "Test: expected " << tex0Frame0 << " to be created for frame 0.");
      checksPassed = false;
    }
    if (vtksys::SystemTools::FileExists(tex0Frame1.c_str(), true /* file */) ||
      vtksys::SystemTools::FileExists(tex0Frame2.c_str(), true /* file */))
    {
      vtkLog(ERROR, "Test: unchanged texture should not have been re-written on frames 1 and 2.");
      checksPassed = false;
    }

    // The second actor's texture only changed on frame 2, so a texture image
    // should exist for frames 0 and 2, but not for frame 1.
    std::string tex1Frame0 = rootname + "_timesteps_tex1_frame0.png";
    std::string tex1Frame1 = rootname + "_timesteps_tex1_frame1.png";
    std::string tex1Frame2 = rootname + "_timesteps_tex1_frame2.png";
    if (!vtksys::SystemTools::FileExists(tex1Frame0.c_str(), true /* file */))
    {
      vtkLog(ERROR, "Test: expected " << tex1Frame0 << " to be created for frame 0.");
      checksPassed = false;
    }
    if (vtksys::SystemTools::FileExists(tex1Frame1.c_str(), true /* file */))
    {
      vtkLog(ERROR, "Test: unchanged texture should not have been re-written on frame 1.");
      checksPassed = false;
    }
    if (!vtksys::SystemTools::FileExists(tex1Frame2.c_str(), true /* file */))
    {
      vtkLog(ERROR, "Test: expected " << tex1Frame2 << " to be created once the texture changed.");
      checksPassed = false;
    }

    // Read the combined archive back and confirm the first mesh really did
    // receive 3 time samples, and that its topology (face counts) differs
    // between the first and last sample, proving topology was actually
    // time-sampled rather than only written once.
    {
      Alembic::AbcGeom::IArchive archive(Alembic::AbcCoreOgawa::ReadArchive(), timestepsFilename);
      Alembic::AbcGeom::IObject top = archive.getTop();
      Alembic::AbcGeom::IXform xform0(top, "xform_0");
      Alembic::AbcGeom::IPolyMesh mesh0(xform0, "mesh_0");
      Alembic::AbcGeom::IPolyMeshSchema& meshSchema = mesh0.getSchema();

      if (meshSchema.getNumSamples() != 3)
      {
        vtkLog(ERROR,
          "Test: expected 3 time samples for mesh_0, got " << meshSchema.getNumSamples() << ".");
        checksPassed = false;
      }
      else
      {
        Alembic::AbcGeom::IPolyMeshSchema::Sample sample0, sample2;
        meshSchema.get(
          sample0, Alembic::Abc::ISampleSelector(static_cast<Alembic::Abc::index_t>(0)));
        meshSchema.get(
          sample2, Alembic::Abc::ISampleSelector(static_cast<Alembic::Abc::index_t>(2)));
        if (sample0.getFaceCounts()->size() == sample2.getFaceCounts()->size())
        {
          vtkLog(ERROR,
            "Test: expected mesh_0 face count to differ between frame 0 and frame 2 since "
            "topology was changed.");
          checksPassed = false;
        }
      }
    }

    // Clean up all files produced by this test.
    vtksys::Glob glob;
    glob.FindFiles(rootname + "*");
    for (const std::string& fileToRemove : glob.GetFiles())
    {
      vtksys::SystemTools::RemoveFile(fileToRemove);
    }

    if (!checksPassed)
    {
      vtkLog(
        ERROR, "Test: one or more checks failed for the single-file multi-timestep export test.");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
