// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTexturedBackground(int argc, char* argv[])
{
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderer, ren);
  VTK_CREATE(vtkConeSource, cone);
  VTK_CREATE(vtkPolyDataMapper, map);
  VTK_CREATE(vtkActor, act);
  VTK_CREATE(vtkTexture, textr);
  VTK_CREATE(vtkJPEGReader, imgReader);
  VTK_CREATE(vtkImageData, image);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");
  imgReader->SetFileName(fname);
  delete[] fname;

  imgReader->Update();

  image->SetDimensions(2, 2, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  image->GetPointData()->GetScalars()->Fill(0);
  textr->SetInputData(image);

  map->SetInputConnection(cone->GetOutputPort(0));
  act->SetMapper(map);
  ren->AddActor(act);
  ren->TexturedBackgroundOn();
  //  ren->GradientBackgroundOn();
  ren->SetBackgroundTexture(textr);
  ren->SetBackground(0.8, 0.4, 0.1);
  ren->SetBackground2(0.1, 0.4, 0.8);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  // Prime the texture cache, then replace its input with an image of a different size.
  win->Render();
  textr->SetInputConnection(imgReader->GetOutputPort(0));
  win->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
