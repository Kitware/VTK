/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTulipReaderClusters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGraphAnnotationLayersFilter.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTulipReader.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTulipReaderClusters(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
    "Data/Infovis/clustered-graph.tlp");
  VTK_CREATE(vtkTulipReader, reader);
  reader->SetFileName(file);
  delete[] file;

  VTK_CREATE(vtkForceDirectedLayoutStrategy, strategy);
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(reader->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  VTK_CREATE(vtkGraphMapper, graphMapper);
  graphMapper->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkActor, graphActor);
  graphActor->SetMapper(graphMapper);

  VTK_CREATE(vtkGraphAnnotationLayersFilter, clusters);
  clusters->SetInputConnection(0, layout->GetOutputPort(0));
  clusters->SetInputConnection(1, reader->GetOutputPort(1));
  clusters->SetScaleFactor(1.2);
  clusters->SetMinHullSizeInWorld(0.02);
  clusters->SetMinHullSizeInDisplay(32);
  clusters->OutlineOn();

  VTK_CREATE(vtkPolyDataMapper, clustersMapper);
  clustersMapper->SetInputConnection(clusters->GetOutputPort());
  clustersMapper->SelectColorArray("Hull color");
  clustersMapper->SetScalarModeToUseCellFieldData();
  clustersMapper->SetScalarVisibility(true);
  VTK_CREATE(vtkActor, clustersActor);
  clustersActor->SetMapper(clustersMapper);

  VTK_CREATE(vtkPolyDataMapper, outlineMapper);
  outlineMapper->SetInputConnection(clusters->GetOutputPort(1));
  VTK_CREATE(vtkActor, outlineActor);
  outlineActor->SetMapper(outlineMapper);
  outlineActor->GetProperty()->SetColor(0.5, 0.7, 0.0);

  VTK_CREATE(vtkRenderer, ren);
  clusters->SetRenderer(ren);
  ren->AddActor(graphActor);
  ren->AddActor(clustersActor);
  ren->AddActor(outlineActor);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->SetMultiSamples(0);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
