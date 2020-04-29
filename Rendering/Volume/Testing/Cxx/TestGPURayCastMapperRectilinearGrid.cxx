/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMapperRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This is a basic test that renders a rectilinear grid dataset with the GPU ray cast volume mapper.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridReader.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------
int TestGPURayCastMapperRectilinearGrid(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/RectGrid2.vtk");

  vtkNew<vtkRectilinearGridReader> reader;
  reader->SetFileName(fname);

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.53, 0.53, 0.83);
  ctf->AddRGBPoint(1.77, 0, 0, 1);
  ctf->AddRGBPoint(3.53, 0, 1, 1);
  ctf->AddRGBPoint(5.2, 0, 1, 0);
  ctf->AddRGBPoint(6.97, 1, 1, 0);
  ctf->AddRGBPoint(8.73, 1, 0, 0);
  ctf->AddRGBPoint(10.39, 0.88, 0, 1);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.7);
  pf->AddPoint(0.35, 0.5);
  pf->AddPoint(10.39, 1);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0);
  gf->AddPoint(0.075, 0);
  gf->AddPoint(0.15, 1);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  vtkNew<vtkVolume> volume;
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->UseJitteringOn();

  mapper->SetInputConnection(reader->GetOutputPort());

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volume->SetProperty(volumeProperty);
  volume->SetMapper(mapper);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren->AddViewProp(volume);
  ren->ResetCamera();

  auto camera = ren->GetActiveCamera();
  camera->Pitch(30);
  ren->ResetCamera();
  camera->Zoom(2);

  renWin->Render();
  delete[] fname;
  return vtkTesting::InteractorEventLoop(argc, argv, iren);
}
