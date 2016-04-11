/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that treatment of multiblock data is correct
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkOSPRayPass.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

#include "vtkOSPRayTestInteractor.h"

int TestOSPRayMultiBlock(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkXMLMultiBlockDataReader> reader = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/many_blocks/many_blocks.vtm");
  reader->SetFileName(fileName);
  reader->Update();

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper=vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  vtkSmartPointer<vtkActor> actor=vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.1,0.1,1.0);
  renWin->SetSize(400,400);
  renWin->Render();

  vtkCamera *cam = renderer->GetActiveCamera();
  cam->SetPosition(1.5,1.5,0.75);

  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();

  renderer->SetPass(ospray);

  renWin->Render();

  vtkSmartPointer<vtkOSPRayTestInteractor> style =
    vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->
    SetPipelineControlPoints((vtkOpenGLRenderer*)renderer.Get(), ospray, NULL);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
