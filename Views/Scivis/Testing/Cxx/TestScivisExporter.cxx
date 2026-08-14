// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises getting a scene out of a view: the image formats and the scene
// formats, what magnification and a transparent background do to a capture, and
// that taking one leaves the view as it was.

#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkScivisExporter.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkTesting.h"

#include <vtksys/SystemTools.hxx>

#include <iostream>
#include <string>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << (msg) << std::endl;                                               \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

namespace
{

// A view showing one sphere, sized so the capture dimensions are predictable.
vtkSmartPointer<vtkScivisView> MakeView()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  auto view = vtkSmartPointer<vtkScivisView>::New();
  view->SetWindowSize(300, 200);
  view->AddRepresentation(surface);
  view->ResetCamera();
  return view;
}

bool WroteSomething(const std::string& path)
{
  return vtksys::SystemTools::FileExists(path) && vtksys::SystemTools::FileLength(path) > 0;
}

int TestImageFormats(const std::string& dir)
{
  auto view = MakeView();
  vtkScivisExporter* exporter = view->GetExporter();

  for (const char* extension : { ".png", ".jpg", ".tif", ".bmp" })
  {
    const std::string path = dir + "/TestScivisExporter" + extension;
    vtksys::SystemTools::RemoveFile(path);
    CHECK(exporter->SaveScreenshot(path.c_str()), std::string("could not write ") + extension);
    CHECK(WroteSomething(path), std::string("nothing was written for ") + extension);
  }

  return EXIT_SUCCESS;
}

int TestSceneFormats(const std::string& dir)
{
  auto view = MakeView();
  vtkScivisExporter* exporter = view->GetExporter();

  for (const char* extension : { ".gltf", ".vrml", ".x3d" })
  {
    const std::string path = dir + "/TestScivisExporterScene" + extension;
    vtksys::SystemTools::RemoveFile(path);
    CHECK(exporter->ExportScene(path.c_str()), std::string("could not export ") + extension);
    CHECK(WroteSomething(path), std::string("nothing was exported for ") + extension);
  }

  // This one names its own files, appending .obj and .mtl to the prefix.
  const std::string obj = dir + "/TestScivisExporterScene.obj";
  vtksys::SystemTools::RemoveFile(obj);
  CHECK(exporter->ExportScene(obj.c_str()), "could not export .obj");
  CHECK(WroteSomething(obj), "nothing was exported for .obj");

  return EXIT_SUCCESS;
}

// An extension nothing can write is refused rather than half done.
int TestUnknownFormats(const std::string& dir)
{
  auto view = MakeView();
  vtkScivisExporter* exporter = view->GetExporter();
  exporter->SetGlobalWarningDisplay(0);

  const std::string image = dir + "/TestScivisExporter.xyz";
  const std::string scene = dir + "/TestScivisExporterScene.xyz";
  CHECK(!exporter->SaveScreenshot(image.c_str()), "an unknown image extension was accepted");
  CHECK(!exporter->ExportScene(scene.c_str()), "an unknown scene extension was accepted");
  CHECK(!exporter->SaveScreenshot(""), "an empty name was accepted");
  CHECK(!exporter->ExportScene(""), "an empty name was accepted for a scene");
  CHECK(!WroteSomething(image) && !WroteSomething(scene), "a refused write left a file behind");

  return EXIT_SUCCESS;
}

// Magnification and transparency describe the capture, and taking one puts the
// view back the way it was.
int TestCaptureSettings()
{
  auto view = MakeView();
  vtkScivisExporter* exporter = view->GetExporter();

  CHECK(exporter->GetMagnification() == 1, "magnification does not start at 1");
  CHECK(!exporter->GetTransparentBackground(), "transparency does not start off");

  vtkSmartPointer<vtkImageData> plain = exporter->CaptureImage();
  CHECK(plain != nullptr, "nothing was captured");
  int plainDimensions[3];
  plain->GetDimensions(plainDimensions);
  CHECK(plain->GetNumberOfScalarComponents() == 3, "an opaque capture is not three components");

  exporter->SetMagnification(2);
  vtkSmartPointer<vtkImageData> larger = exporter->CaptureImage();
  CHECK(larger != nullptr, "nothing was captured at twice the size");
  int largerDimensions[3];
  larger->GetDimensions(largerDimensions);
  CHECK(
    largerDimensions[0] == 2 * plainDimensions[0] && largerDimensions[1] == 2 * plainDimensions[1],
    "magnification did not double the capture");

  // Below the smallest useful value, the clamp holds.
  exporter->SetMagnification(0);
  CHECK(exporter->GetMagnification() == 1, "magnification was allowed below 1");

  vtkRenderer* renderer = view->GetRenderer();
  const double alpha = renderer->GetBackgroundAlpha();
  const bool gradient = renderer->GetGradientBackground();

  exporter->TransparentBackgroundOn();
  vtkSmartPointer<vtkImageData> transparent = exporter->CaptureImage();
  CHECK(transparent != nullptr, "nothing was captured with a transparent background");
  CHECK(transparent->GetNumberOfScalarComponents() == 4,
    "a transparent capture does not carry an alpha channel");
  CHECK(renderer->GetBackgroundAlpha() == alpha && renderer->GetGradientBackground() == gradient,
    "a transparent capture left the view's background changed");

  return EXIT_SUCCESS;
}

// The exporter is reached through the view, but can outlive it.
int TestOutlivingTheView()
{
  vtkSmartPointer<vtkScivisExporter> exporter;
  {
    auto view = MakeView();
    exporter = view->GetExporter();
  }
  exporter->SetGlobalWarningDisplay(0);
  CHECK(exporter->CaptureImage() == nullptr, "capturing without a view did not return null");
  CHECK(!exporter->ExportScene("orphan.gltf"), "exporting without a view did not fail");

  return EXIT_SUCCESS;
}

}

int TestScivisExporter(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  for (int i = 0; i < argc; ++i)
  {
    testing->AddArgument(argv[i]);
  }
  const std::string dir = testing->GetTempDirectory();

  if (TestImageFormats(dir) != EXIT_SUCCESS || TestSceneFormats(dir) != EXIT_SUCCESS ||
    TestUnknownFormats(dir) != EXIT_SUCCESS || TestCaptureSettings() != EXIT_SUCCESS ||
    TestOutlivingTheView() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
