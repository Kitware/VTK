/*=========================================================================

  Program:   Visualization Toolkit
  Module:    X3DTest.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkGlyph3D.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkX3DExporter.h"

int X3DTest(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkConeSource> cone;
  cone->SetResolution(6);

  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkNew<vtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  vtkNew<vtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(spikeActor);
  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  renWin->Render();

  vtkNew<vtkX3DExporter> exporter;
  exporter->SetInput(renWin);
  exporter->SetFileName("testX3DExporter.x3d");
  exporter->Update();
  exporter->Write();
  exporter->Print(std::cout);

  renderer->RemoveActor(sphereActor);
  renderer->RemoveActor(spikeActor);

  // now try the same with a composite dataset.
  vtkNew<vtkMultiBlockDataSet> mb;
  mb->SetBlock(0, glyph->GetOutputDataObject(0));
  mb->GetMetaData(0u)->Set(vtkMultiBlockDataSet::NAME(), "Spikes");
  mb->SetBlock(1, sphere->GetOutputDataObject(0));
  mb->GetMetaData(1u)->Set(vtkMultiBlockDataSet::NAME(), "Sphere");

  vtkNew<vtkPolyDataMapper> mbMapper;
  mbMapper->SetInputDataObject(mb);

  vtkNew<vtkActor> mbActor;
  mbActor->SetMapper(mbMapper);
  renderer->AddActor(mbActor);

  renWin->Render();
  exporter->SetFileName("testX3DExporter-composite.x3d");
  exporter->Update();
  exporter->Write();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
