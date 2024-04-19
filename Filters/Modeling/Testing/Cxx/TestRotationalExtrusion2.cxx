// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkLineSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRotationalExtrusionFilter.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestRotationalExtrusion2(int argc, char* argv[])
{
  // Create a line source
  vtkNew<vtkLineSource> line;
  line->SetPoint1(0., 1., 0.);
  line->SetPoint2(0., 1., 2.);
  line->SetResolution(10);

  // Create mapper for line segment
  vtkNew<vtkPolyDataMapper> lineMapper;
  lineMapper->SetInputConnection(line->GetOutputPort());

  // Create actor for line segment
  vtkNew<vtkActor> lineActor;
  lineActor->SetMapper(lineMapper);
  lineActor->GetProperty()->SetLineWidth(5);
  lineActor->GetProperty()->SetColor(0., .749, 1.); // deep sky blue

  // Create 3/4 of a cylinder by rotational extrusion
  vtkNew<vtkRotationalExtrusionFilter> lineSweeper;
  lineSweeper->SetResolution(20);
  lineSweeper->SetInputConnection(line->GetOutputPort());
  lineSweeper->SetAngle(270);
  lineSweeper->SetRotationAxis(1, 0, 1);

  // Create normals for smooth rendering
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(lineSweeper->GetOutputPort());

  // Create mapper for surface representation
  vtkNew<vtkPolyDataMapper> cylMapper;
  cylMapper->SetInputConnection(normals->GetOutputPort());
  cylMapper->SetResolveCoincidentTopologyToPolygonOffset();

  // Create mapper for wireframe representation
  vtkNew<vtkPolyDataMapper> cylMapperW;
  cylMapperW->SetInputConnection(lineSweeper->GetOutputPort());
  cylMapperW->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for surface representation
  vtkNew<vtkActor> cylActor;
  cylActor->SetMapper(cylMapper);
  cylActor->GetProperty()->SetRepresentationToSurface();
  cylActor->GetProperty()->SetInterpolationToGouraud();
  cylActor->GetProperty()->SetColor(1., .3882, .2784); // tomato

  // Create actor for wireframe representation
  vtkNew<vtkActor> cylActorW;
  cylActorW->SetMapper(cylMapperW);
  cylActorW->GetProperty()->SetRepresentationToWireframe();
  cylActorW->GetProperty()->SetColor(0., 0., 0.);
  cylActorW->GetProperty()->SetAmbient(1.);
  cylActorW->GetProperty()->SetDiffuse(0.);
  cylActorW->GetProperty()->SetSpecular(0.);

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> ren1;
  ren1->AddActor(lineActor);
  ren1->AddActor(cylActor);
  ren1->AddActor(cylActorW);
  ren1->SetBackground(1., 1., 1.);

  // Create a renderWindow
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  // Create a good view angle
  vtkNew<vtkCamera> camera;
  camera->SetPosition(7.395789993306532, 9.691575562803443, 3.026104158779236);
  camera->SetFocalPoint(1.4814705502887429, -0.3407913255461607, 0.8067055831393791);
  camera->SetViewUp(-0.15295139734386498, -0.12664956167569574, 0.9800845670542483);
  ren1->SetActiveCamera(camera);

  // Create interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
