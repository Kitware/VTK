// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkArrowSource.h"
#include "vtkCamera.h"
#include "vtkExtractGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
// #define USE_FILTER

#ifdef USE_FILTER
#include "vtkGlyph3D.h"
#else
#include "vtkGlyph3DMapper.h"
#endif

// from Graphics/Testing/Python/glyphComb.py

int TestGlyph3DMapperArrow(int argc, char* argv[])
{
  vtkNew<vtkMultiBlockPLOT3DReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  reader->SetXYZFileName(fname);
  delete[] fname;
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");
  reader->SetQFileName(fname);
  delete[] fname;
  reader->SetScalarFunctionNumber(100);
  reader->SetVectorFunctionNumber(202);
  reader->Update();

  vtkNew<vtkExtractGrid> eg;
  eg->SetInputData(reader->GetOutput()->GetBlock(0));
  eg->SetSampleRate(4, 4, 4);
  eg->Update();

  cout << "eg pts=" << eg->GetOutput()->GetNumberOfPoints() << endl;
  cout << "eg cells=" << eg->GetOutput()->GetNumberOfCells() << endl;

  // create simple poly data so we can apply glyph
  vtkNew<vtkArrowSource> arrow;
  arrow->Update();
  cout << "pts=" << arrow->GetOutput()->GetNumberOfPoints() << endl;
  cout << "cells=" << arrow->GetOutput()->GetNumberOfCells() << endl;

#ifdef USE_FILTER
  vtkNew<vtkGlyph3D> glypher;
#else
  vtkNew<vtkGlyph3DMapper> glypher;
#endif
  glypher->SetInputConnection(eg->GetOutputPort());
  glypher->SetSourceConnection(arrow->GetOutputPort());
  glypher->SetScaleFactor(2.0);

#ifdef USE_FILTER
  vtkNew<vtkPolyDataMapper> glyphMapper;
  glyphMapper->SetInputConnection(glypher->GetOutputPort());
#endif

  vtkNew<vtkActor> glyphActor;
#ifdef USE_FILTER
  glyphActor->SetMapper(glyphMapper);
#else
  glyphActor->SetMapper(glypher);
#endif

  // Create the rendering stuff

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(glyphActor);
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);

  auto* cam = ren->GetActiveCamera();
  cam->SetClippingRange(3.95297, 50);
  cam->SetFocalPoint(8.88908, 0.595038, 29.3342);
  cam->SetPosition(-12.3332, 31.7479, 41.2387);
  cam->SetViewUp(0.060772, -0.319905, 0.945498);

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "first frame: " << timer->GetElapsedTime() << " seconds" << endl;

  //  ren->GetActiveCamera()->Zoom(1.5);
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "second frame: " << timer->GetElapsedTime() << " seconds" << endl;

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
