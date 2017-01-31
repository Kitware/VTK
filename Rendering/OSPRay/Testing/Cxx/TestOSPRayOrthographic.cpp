/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayOrthographic.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can switch between scivis and pathtracer modes
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkPLYReader.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#include "vtkOSPRayTestInteractor.h"

int TestOSPRayOrthographic(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkSmartPointer<vtkPLYReader> polysource = vtkSmartPointer<vtkPLYReader>::New();
  polysource->SetFileName(fileName);

  //TODO: ospray acts strangely without these such that Diff and Spec are not 0..255 instead of 0..1
  vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());
  //normals->ComputePointNormalsOn();
  //normals->ComputeCellNormalsOff();

  vtkSmartPointer<vtkPolyDataMapper> mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkSmartPointer<vtkActor> actor=vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.1,0.1,1.0);
  renWin->SetSize(400,400);
  renWin->Render();

  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);

  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetParallelProjection(1);
  renWin->Render();

  vtkSmartPointer<vtkOSPRayTestInteractor> style =
    vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints((vtkOpenGLRenderer*)renderer.Get(), ospray, NULL);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
