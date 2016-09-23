/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayImplicits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that sizing of implicits spheres and cylinders for
// points and lines works as expected.
//
// The command line arguments are:
// -I         => run in interactive mode; unless this is used, the program will
//               not allow interaction and exit.
//               In interactive mode it responds to the keys listed
//               vtkOSPRayTestInteractor.h
// -GL        => users OpenGL instead of OSPRay to render

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayPass.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkScalarsToColors.h"
#include "vtkShrinkFilter.h"
#include "vtkSmartPointer.h"

#include <vector>
#include <string>

#include "vtkOSPRayTestInteractor.h"

int TestOSPRayImplicits(int argc, char* argv[])
{
  bool useGL = false;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      useGL = true;
    }
  }

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.75,0.75,0.75);
  renWin->SetSize(600,550);

  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  if (!useGL)
  {
    renderer->SetPass(ospray);
  }

  vtkSmartPointer<vtkRTAnalyticSource> wavelet = vtkSmartPointer<vtkRTAnalyticSource>::New();
  wavelet->SetWholeExtent(-10,10,-10,10,-10,10);
  wavelet->SetSubsampleRate(5);
  wavelet->Update();
  //use a more predictable array
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName("testarray1");
  da->SetNumberOfComponents(1);
  vtkDataSet * ds = wavelet->GetOutput();
  ds->GetPointData()->AddArray(da);
  int np = ds->GetNumberOfPoints();
  for (int i = 0; i < np; i++)
  {
    da->InsertNextValue((double)i/(double)np);
  }

  vtkSmartPointer<vtkDataSetSurfaceFilter> surfacer = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfacer->SetInputData(ds);
  vtkSmartPointer<vtkShrinkFilter> shrinker = vtkSmartPointer<vtkShrinkFilter>::New();
  shrinker->SetShrinkFactor(0.5);
  shrinker->SetInputConnection(surfacer->GetOutputPort());

  //measure it for placements
  shrinker->Update();
  double *bds = vtkDataSet::SafeDownCast(shrinker->GetOutputDataObject(0))->GetBounds();
  double x0 = bds[0];
  double y0 = bds[2];
  double z0 = bds[4];
  double dx = (bds[1]-bds[0])*1.2;
  double dy = (bds[3]-bds[2])*1.2;
  double dz = (bds[5]-bds[4])*1.2;

  //make points, point rep works too but only gets outer shell
  vtkSmartPointer<vtkGlyphSource2D> glyph = vtkSmartPointer<vtkGlyphSource2D>::New();
  glyph->SetGlyphTypeToVertex();
  vtkSmartPointer<vtkGlyph3D> glyphFilter = vtkSmartPointer<vtkGlyph3D>::New();
  glyphFilter->SetInputConnection(shrinker->GetOutputPort());
  glyphFilter->SetSourceConnection(glyph->GetOutputPort());

  vtkSmartPointer<vtkExtractEdges> edgeFilter =
    vtkSmartPointer<vtkExtractEdges>::New();
  edgeFilter->SetInputConnection(shrinker->GetOutputPort());

  //spheres ///////////////////////
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*0, y0+dy*0, z0+dz*0);
  vtkOSPRayTestInteractor::AddName("Points default");

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*1, y0+dy*0, z0+dz*0);
  actor->GetProperty()->SetPointSize(5);
  vtkOSPRayTestInteractor::AddName("Points SetPointSize()");

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*2, y0+dy*0, z0+dz*0);
  vtkInformation *mapInfo = mapper->GetInformation();
  mapInfo = mapper->GetInformation();
  mapInfo->Set(vtkOSPRayActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(vtkOSPRayActorNode::SCALE_ARRAY_NAME(),"testarray1");
  vtkOSPRayTestInteractor::AddName("Points SCALE_ARRAY");

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*3, y0+dy*0, z0+dz*0);
  mapInfo = mapper->GetInformation();
  mapInfo->Set(vtkOSPRayActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(vtkOSPRayActorNode::SCALE_ARRAY_NAME(),"testarray1");
  vtkSmartPointer<vtkPiecewiseFunction> scaleFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  scaleFunction->AddPoint(0.00, 0.0);
  scaleFunction->AddPoint(0.50, 0.0);
  scaleFunction->AddPoint(0.51, 0.1);
  scaleFunction->AddPoint(1.00, 1.2);
  mapInfo->Set(vtkOSPRayActorNode::SCALE_FUNCTION(),
               scaleFunction);
  vtkOSPRayTestInteractor::AddName("Points SCALE_FUNCTION on SCALE_ARRAY");


  // cylinders ////////////////
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*0, y0+dy*2, z0+dz*0);
  vtkOSPRayTestInteractor::AddName("Wireframe default");

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*1, y0+dy*2, z0+dz*0);
  actor->GetProperty()->SetLineWidth(5);
  vtkOSPRayTestInteractor::AddName("Wireframe LineWidth");

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*2, y0+dy*2, z0+dz*0);
  vtkOSPRayActorNode::SetEnableScaling(1, actor);
  vtkOSPRayActorNode::SetScaleArrayName("testarray1",actor);
  vtkOSPRayTestInteractor::AddName("Wireframe SCALE_ARRAY");

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*3, y0+dy*2, z0+dz*0);
  mapInfo = mapper->GetInformation();
  mapInfo->Set(vtkOSPRayActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(vtkOSPRayActorNode::SCALE_ARRAY_NAME(), "testarray1");
  scaleFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  scaleFunction->AddPoint(0.00, 0.0);
  scaleFunction->AddPoint(0.50, 0.0);
  scaleFunction->AddPoint(0.51, 0.1);
  scaleFunction->AddPoint(1.00, 1.2);
  mapInfo->Set(vtkOSPRayActorNode::SCALE_FUNCTION(),
               scaleFunction);
  vtkOSPRayTestInteractor::AddName("Wireframe SCALE_FUNCTION on SCALE_ARRAY");

  // reference values shown as colors /////////////////
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(surfacer->GetOutputPort());
  surfacer->Update();
  mapper->ScalarVisibilityOn();
  mapper->CreateDefaultLookupTable();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("testarray1");
  double *range = surfacer->GetOutput()->GetPointData()->GetArray("testarray1")->GetRange();
  mapper->SetScalarRange(range);
  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToSurface();

  renderer->AddActor(actor);
  actor->SetPosition(x0+dx*2, y0+dy*1, z0+dz*0);
  vtkOSPRayTestInteractor::AddName("Reference values as colors");

  //just show it //////////////////
  renWin->Render();
  renderer->ResetCamera();

  vtkSmartPointer<vtkOSPRayTestInteractor> style =
    vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->
    SetPipelineControlPoints((vtkOpenGLRenderer*)renderer.Get(), ospray, NULL);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
