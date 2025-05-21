// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers rendering translucent materials with depth peeling
// technique.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCubeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkImageGridSource.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

// if not defined, we use spherical glyphs (slower) instead of cubic
// glyphs (faster)
// #define VTK_TEST_OPACITY_CUBE

int TestOpacity(int argc, char* argv[])
{
  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  if (renWin->IsA("vtkWebAssemblyOpenGLRenderWindow"))
  {
    // WebAssembly OpenGL requires additional steps for dual depth peeling. See
    // TestFramebufferPass.cxx for details.
    std::cout << "Skipping test with dual-depth peeling for WebAssembly OpenGL\n";
    return VTK_SKIP_RETURN_CODE;
  }
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // We create a bunch of translucent spheres with an opaque plane in
  // the middle
  // we create a uniform grid and glyph it with a spherical shape.

  // Create the glyph source
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1);
  sphere->SetCenter(0.0, 0.0, 0.0);
  sphere->SetThetaResolution(10);
  sphere->SetPhiResolution(10);
  sphere->SetLatLongTessellation(0);

  vtkNew<vtkCubeSource> cube;
  cube->SetXLength(1.0);
  cube->SetYLength(1.0);
  cube->SetZLength(1.0);
  cube->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkImageGridSource> grid;
  grid->SetGridSpacing(1, 1, 1);
  grid->SetGridOrigin(0, 0, 0);
  grid->SetLineValue(1.0); // white
  grid->SetFillValue(0.5); // gray
  grid->SetDataScalarTypeToUnsignedChar();
  grid->SetDataExtent(0, 10, 0, 10, 0, 10);
  grid->SetDataSpacing(0.1, 0.1, 0.1);
  grid->SetDataOrigin(0.0, 0.0, 0.0);
  grid->Update(); // to get the range

  double range[2];
  grid->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(0, grid->GetOutputPort(0));
#ifdef VTK_TEST_OPACITY_CUBE
  glyph->SetSourceConnection(cube->GetOutputPort(0));
#else
  glyph->SetSourceConnection(sphere->GetOutputPort(0));
#endif
  glyph->SetScaling(1); // on
  glyph->SetScaleModeToScaleByScalar();
  glyph->SetColorModeToColorByScale();
  glyph->SetScaleFactor(0.05);
  glyph->SetRange(range);
  glyph->SetOrient(0);
  glyph->SetClamping(0);
  glyph->SetVectorModeToUseVector();
  glyph->SetIndexModeToOff();
  glyph->SetGeneratePointIds(0);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(glyph->GetOutputPort(0));

  // This creates a blue to red lut.
  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.667, 0.0);
  mapper->SetLookupTable(lut);
  mapper->SetScalarRange(range);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  vtkNew<vtkProperty> property;
  property->SetOpacity(0.2);
  property->SetColor(0.0, 1.0, 0.0);
  actor->SetProperty(property);

  vtkNew<vtkPlaneSource> plane;
  plane->SetCenter(0.5, 0.5, 0.5);

  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(0, plane->GetOutputPort(0));

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  renderer->AddActor(planeActor);

  vtkNew<vtkProperty> planeProperty;
  planeProperty->SetOpacity(1.0);
  planeProperty->SetColor(1.0, 0.0, 0.0);
  planeActor->SetProperty(planeProperty);
  planeProperty->SetBackfaceCulling(0);
  planeProperty->SetFrontfaceCulling(0);

  renderer->SetUseDepthPeeling(1);
  // reasonable depth peeling settings
  // no more than 50 layers of transluceny
  renderer->SetMaximumNumberOfPeels(50);
  // stop when less than 2 in 1000 pixels changes
  renderer->SetOcclusionRatio(0.002);

  property->SetBackfaceCulling(1);
  property->SetFrontfaceCulling(0);

  // Standard testing code.
  renderer->SetBackground(0.0, 0.5, 0.0);
  renWin->SetSize(300, 300);
  renWin->Render();

  if (renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout << "depth peeling was used" << endl;
  }
  else
  {
    cout << "depth peeling was not used (alpha blending instead)" << endl;
  }
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
