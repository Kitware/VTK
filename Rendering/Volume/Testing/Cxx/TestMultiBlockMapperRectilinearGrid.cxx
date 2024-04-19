// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * \brief Tests vtkMultiBlockDataSet rendering. Uses the exposed VectorMode
 * API of vtkSmartVolumeMapper to render component X of the array data.
 *
 */

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataSet.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRectilinearGridReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPUnstructuredGridReader.h"

int TestMultiBlockMapperRectilinearGrid(int argc, char* argv[])
{
  vtkNew<vtkRectilinearGridReader> reader;
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/RectGrid2.vtk");
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  vtkNew<vtkMultiBlockDataGroupFilter> grouper;
  grouper->AddInputConnection(reader->GetOutputPort());

  vtkNew<vtkMultiBlockVolumeMapper> mapper;
  mapper->SetInputConnection(grouper->GetOutputPort());
  // mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.53, 0.53, 0.83);
  ctf->AddRGBPoint(1.77, 0, 0, 1);
  ctf->AddRGBPoint(3.53, 0, 1, 1);
  ctf->AddRGBPoint(5.2, 0, 1, 0);
  ctf->AddRGBPoint(6.97, 1, 1, 0);
  ctf->AddRGBPoint(8.73, 1, 0, 0);
  ctf->AddRGBPoint(10.39, 0.88, 0, 1);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0);
  pf->AddPoint(0.2, 1);
  pf->AddPoint(3, 0.5);
  pf->AddPoint(10.39, 1);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pf);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(401, 400);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  ren->AddVolume(volume);
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(0);
  ren->GetActiveCamera()->Roll(-65);
  ren->GetActiveCamera()->Elevation(-45);
  ren->GetActiveCamera()->Zoom(1.2);
  renWin->Render();

  // initialize render loop
  int const retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
  }

  return !retVal;
}
