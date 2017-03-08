/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiBlockMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * \brief Tests vtkMultiBlockDataSet rendering. Uses the exposed VectorMode
 * API of vtkSmartVolumeMapper to render component X of the array data.
 *
 */

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataSet.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPUnstructuredGridReader.h"


int TestMultiBlockMapper(int argc, char *argv[])
{
  //cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
    "Data/headmr3blocks/headmr3blocks.vtm");
  reader->SetFileName(fileName);
  reader->Update();

  vtkNew<vtkMultiBlockVolumeMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SelectScalarArray("MetaImage");
  mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
  mapper->SetJitteringResolution(401, 400); // alleviate seam artifacts

  vtkNew<vtkColorTransferFunction> color;
  color->AddHSVPoint(1.0, 0.095, 0.33, 0.82);
  color->AddHSVPoint(53.3, 0.04, 0.7, 0.63);
  color->AddHSVPoint(256, 0.095, 0.33, 0.82);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(4.48, 0.0);
  opacity->AddPoint(43.116, 0.35);
  opacity->AddPoint(641.0, 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->SetColor(color.GetPointer());
  property->SetScalarOpacity(opacity.GetPointer());
  property->SetInterpolationTypeToLinear();
  property->ShadeOn();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(property.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(401, 400);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(0);
  ren->GetActiveCamera()->Roll(-65);
  ren->GetActiveCamera()->Elevation(-45);
  ren->GetActiveCamera()->Zoom(1.2);
  renWin->Render();

  // initialize render loop
  int const retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
  }

  return !retVal;
}
