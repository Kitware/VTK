/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiBlockUnstructuredGridVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProjectedTetrahedraMapper.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkExodusIIReader.h"
#include "vtkLogger.h"
#include "vtkMultiBlockUnstructuredGridVolumeMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestMultiBlockUnstructuredGridVolumeMapper(int argc, char* argv[])
{
  char* CfileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  std::string filename(CfileName);
  delete[] CfileName;

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  vtkLog(INFO, "Loading " << filename);
  vtkNew<vtkExodusIIReader> reader;
  if (!reader->CanReadFile(filename.c_str()))
  {
    vtkLog(ERROR, "Error: cannot open file " << filename);
    return EXIT_FAILURE;
  }
  reader->SetFileName(filename.c_str());
  reader->UpdateInformation();
  reader->SetTimeStep(25);
  reader->SetAllArrayStatus(vtkExodusIIReader::NODAL, 1);

  vtkNew<vtkDataSetTriangleFilter> trifilter;
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  vtkNew<vtkMultiBlockUnstructuredGridVolumeMapper> volumeMapper;
  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkVolume> volume;
  // Create the standard renderer, render window, and interactor.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  ren->SetBackground(255, 255, 255);
  renWin->AddRenderer(ren);

  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);

  // check for driver support
  renWin->Render();

  // Make sure we have only tetrahedra.
  trifilter->SetInputConnection(reader->GetOutputPort());

  // Create transfer mapping scalar value to opacity.
  opacityTransferFunction->AddPoint(40, 0.3);
  opacityTransferFunction->AddPoint(1500, 0.45);
  opacityTransferFunction->AddPoint(3000, 0.6);
  opacityTransferFunction->AddPoint(4500, 0.8);
  opacityTransferFunction->AddPoint(5600, 0.9);

  // Create transfer mapping scalar value to color.
  colorTransferFunction->AddRGBPoint(40.0, 0.231, 0.298, 0.753);
  colorTransferFunction->AddRGBPoint(100.0, 0.865, 0.865, 0.865);
  colorTransferFunction->AddRGBPoint(400.0, 0.706, 0.016, 0.149);

  // The property describes how the data will look.
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());
  volumeMapper->SelectScalarArray("VEL");
  volumeMapper->SetScalarMode(3); // VTK_SCALAR_MODE_USE_POINT_FIELD_DATA

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddVolume(volume);

  renWin->SetSize(300, 300);

  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(-41.0, 14.5, -34.0);
  ren->GetActiveCamera()->SetFocalPoint(2.68, 1.63, -8.77);
  ren->GetActiveCamera()->SetViewUp(0.248, 0.966, 0.066);
  ren->GetActiveCamera()->SetViewAngle(30);
  ren->GetActiveCamera()->Azimuth(20.0);
  ren->GetActiveCamera()->Elevation(10.0);
  ren->GetActiveCamera()->Zoom(1.5);

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
