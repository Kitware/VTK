// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that sizing of implicits spheres and cylinders for
// points and lines works as expected.
//
// The command line arguments are:
// -I         => run in interactive mode; unless this is used, the program will
//               not allow interaction and exit.
//               In interactive mode it responds to the keys listed
//               vtkAnariTestInteractor.h
// -GL        => uses OpenGL instead of Anari to render

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkShrinkFilter.h"

#include "vtkAnariActorNode.h"
#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"

#include <string>
#include <vector>

int TestAnariImplicits(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.7, 0.7, 0.7);
  renWin->SetSize(600, 550);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariImplicits");

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetSubsampleRate(5);
  wavelet->Update();
  // use a more predictable array
  vtkNew<vtkDoubleArray> da;
  da->SetName("testarray1");
  da->SetNumberOfComponents(1);
  vtkDataSet* ds = wavelet->GetOutput();
  ds->GetPointData()->AddArray(da);
  int np = ds->GetNumberOfPoints();
  for (int i = 0; i < np; i++)
  {
    da->InsertNextValue((double)i / (double)np);
  }

  vtkNew<vtkDataSetSurfaceFilter> surfacer;
  surfacer->SetInputData(ds);
  vtkNew<vtkShrinkFilter> shrinker;
  shrinker->SetShrinkFactor(1.0);
  shrinker->SetInputConnection(surfacer->GetOutputPort());

  // measure it for placements
  shrinker->Update();
  const double* bds = vtkDataSet::SafeDownCast(shrinker->GetOutputDataObject(0))->GetBounds();
  double x0 = bds[0];
  double y0 = bds[2];
  double z0 = bds[4];
  double dx = (bds[1] - bds[0]) * 1.2;
  double dy = (bds[3] - bds[2]) * 1.2;
  double dz = (bds[5] - bds[4]) * 1.2;

  // make points, point rep works too but only gets outer shell
  vtkNew<vtkGlyphSource2D> glyph;
  glyph->SetGlyphTypeToVertex();
  vtkNew<vtkGlyph3D> glyphFilter;
  glyphFilter->SetInputConnection(shrinker->GetOutputPort());
  glyphFilter->SetSourceConnection(glyph->GetOutputPort());

  vtkNew<vtkExtractEdges> edgeFilter;
  edgeFilter->SetInputConnection(shrinker->GetOutputPort());

  // spheres ///////////////////////
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(glyphFilter->GetOutputPort());
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor1);
  actor1->SetPosition(x0 + dx * 0, y0 + dy * 0, z0 + dz * 0);
  actor1->GetProperty()->SetPointSize(4.0f);
  vtkAnariTestInteractor::AddName("Points default");

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(glyphFilter->GetOutputPort());
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor2);
  actor2->SetPosition(x0 + dx * 1, y0 + dy * 0, z0 + dz * 0);
  actor2->GetProperty()->SetPointSize(5.0f);
  vtkAnariTestInteractor::AddName("Points SetPointSize()");

  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(glyphFilter->GetOutputPort());
  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor3);
  actor3->SetPosition(x0 + dx * 2, y0 + dy * 0, z0 + dz * 0);
  vtkInformation* mapInfo = mapper3->GetInformation();
  mapInfo->Set(vtkAnariActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(vtkAnariActorNode::SCALE_ARRAY_NAME(), "testarray1");
  vtkAnariTestInteractor::AddName("Points SCALE_ARRAY");

  vtkNew<vtkPolyDataMapper> mapper4;
  mapper4->SetInputConnection(glyphFilter->GetOutputPort());
  vtkNew<vtkActor> actor4;
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor4);
  actor4->SetPosition(x0 + dx * 3, y0 + dy * 0, z0 + dz * 0);
  mapInfo = mapper4->GetInformation();
  mapInfo->Set(vtkAnariActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(vtkAnariActorNode::SCALE_ARRAY_NAME(), "testarray1");
  vtkNew<vtkPiecewiseFunction> scaleFunction1;
  scaleFunction1->AddPoint(0.00, 0.0);
  scaleFunction1->AddPoint(0.50, 0.0);
  scaleFunction1->AddPoint(0.51, 0.1);
  scaleFunction1->AddPoint(1.00, 1.2);
  mapInfo->Set(vtkAnariActorNode::SCALE_FUNCTION(), scaleFunction1);
  vtkAnariTestInteractor::AddName("Points SCALE_FUNCTION on SCALE_ARRAY");

  // cylinders ////////////////
  vtkNew<vtkPolyDataMapper> mapper5;
  mapper5->SetInputConnection(edgeFilter->GetOutputPort());
  vtkNew<vtkActor> actor5;
  actor5->SetMapper(mapper5);
  actor5->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor5);
  actor5->SetPosition(x0 + dx * 0, y0 + dy * 2, z0 + dz * 0);
  actor5->GetProperty()->SetLineWidth(2.0f);
  vtkAnariTestInteractor::AddName("Wireframe default");

  vtkNew<vtkPolyDataMapper> mapper6;
  mapper6->SetInputConnection(edgeFilter->GetOutputPort());
  vtkNew<vtkActor> actor6;
  actor6->SetMapper(mapper6);
  actor6->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor6);
  actor6->SetPosition(x0 + dx * 1, y0 + dy * 2, z0 + dz * 0);
  actor6->GetProperty()->SetLineWidth(5.0f);
  vtkAnariTestInteractor::AddName("Wireframe LineWidth");

  vtkNew<vtkPolyDataMapper> mapper7;
  mapper7->SetInputConnection(edgeFilter->GetOutputPort());
  vtkNew<vtkActor> actor7;
  actor7->SetMapper(mapper7);
  actor7->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor7);
  actor7->SetPosition(x0 + dx * 2, y0 + dy * 2, z0 + dz * 0);
  vtkAnariActorNode::SetEnableScaling(1, actor7);
  vtkAnariActorNode::SetScaleArrayName("testarray1", actor7);
  vtkAnariTestInteractor::AddName("Wireframe SCALE_ARRAY");

  vtkNew<vtkPolyDataMapper> mapper8;
  mapper8->SetInputConnection(edgeFilter->GetOutputPort());
  vtkNew<vtkActor> actor8;
  actor8->SetMapper(mapper8);
  actor8->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor8);
  actor8->SetPosition(x0 + dx * 3, y0 + dy * 2, z0 + dz * 0);
  mapInfo = mapper8->GetInformation();
  mapInfo->Set(vtkAnariActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(vtkAnariActorNode::SCALE_ARRAY_NAME(), "testarray1");
  vtkNew<vtkPiecewiseFunction> scaleFunction2;
  scaleFunction2->AddPoint(0.00, 0.0);
  scaleFunction2->AddPoint(0.50, 0.0);
  scaleFunction2->AddPoint(0.51, 0.1);
  scaleFunction2->AddPoint(1.00, 1.2);
  mapInfo->Set(vtkAnariActorNode::SCALE_FUNCTION(), scaleFunction2);
  vtkAnariTestInteractor::AddName("Wireframe SCALE_FUNCTION on SCALE_ARRAY");

  // reference values shown as colors /////////////////
  vtkNew<vtkPolyDataMapper> mapper9;
  mapper9->SetInputConnection(surfacer->GetOutputPort());
  surfacer->Update();
  mapper9->ScalarVisibilityOn();
  mapper9->CreateDefaultLookupTable();
  mapper9->SetColorModeToMapScalars();
  mapper9->SetScalarModeToUsePointFieldData();
  mapper9->SelectColorArray("testarray1");
  double* range = surfacer->GetOutput()->GetPointData()->GetArray("testarray1")->GetRange();
  mapper9->SetScalarRange(range);
  vtkNew<vtkActor> actor9;
  actor9->SetMapper(mapper9);
  actor9->GetProperty()->SetRepresentationToSurface();

  renderer->AddActor(actor9);
  actor9->SetPosition(x0 + dx * 2, y0 + dy * 1, z0 + dz * 0);
  vtkAnariTestInteractor::AddName("Reference values as colors");

  // just show it //////////////////
  renWin->Render();
  renderer->ResetCamera();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkAnariTestInteractor> style;
    style->SetPipelineControlPoints(renderer, anariPass, nullptr);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(renderer);

    iren->Start();
  }

  return !retVal;
}
