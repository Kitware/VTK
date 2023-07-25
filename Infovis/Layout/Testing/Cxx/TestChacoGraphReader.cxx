// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkActor.h"
#include "vtkChacoGraphReader.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestChacoGraphReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/small.graph");
  VTK_CREATE(vtkChacoGraphReader, reader);
  reader->SetFileName(file);
  delete[] file;
  reader->Update();

  VTK_CREATE(vtkCircularLayoutStrategy, strategy);
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(reader->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->SetMultiSamples(0);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}
