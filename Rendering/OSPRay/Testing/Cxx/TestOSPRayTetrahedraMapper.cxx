/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This is a basic test that creates and volume renders the wavelet dataset.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkTestErrorObserver.h"
#include "vtkOSPRayPass.h"
#include "vtkStructuredPointsReader.h"
#include "vtkSLCReader.h"
#include "vtkThreshold.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include <vtkTestUtilities.h>

static const char * TestOSPRayTetrahedraMapperLog =
"# StreamVersion 1\n"
"EnterEvent 299 0 0 0 0 0 0\n"
"MouseMoveEvent 299 0 0 0 0 0 0\n"
"MouseMoveEvent 298 2 0 0 0 0 0\n"
"MouseMoveEvent 297 4 0 0 0 0 0\n"
"MouseMoveEvent 297 6 0 0 0 0 0\n"
"MouseMoveEvent 296 8 0 0 0 0 0\n"
"LeaveEvent 399 -8 0 0 0 0 0\n"
;


int TestOSPRayTetrahedraMapper(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }

  // Create the reader for the data
  // This is the data the will be volume rendered
  vtkNew<vtkStructuredPointsReader> reader;
  const char* file1 = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/ironProt.vtk");
  reader->SetFileName(file1);

  // create a reader for the other data that will
  // be contoured and displayed as a polygonal mesh
  vtkNew<vtkSLCReader> reader2;
  const char* file2 = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/neghip.slc");
  reader2->SetFileName(file2);

  // convert from vtkImageData to vtkUnstructuredGrid, remove
  // any cells where all values are below 80
  vtkNew<vtkThreshold> thresh;
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(reader->GetOutputPort());

  // make sure we have only tetrahedra
  vtkNew<vtkDataSetTriangleFilter> trifilter;
  trifilter->SetInputConnection(thresh->GetOutputPort());

  // Create transfer mapping scalar value to opacity
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  opacityTransferFunction->AddPoint(80, 0.0);
  opacityTransferFunction->AddPoint(120, 0.2);
  opacityTransferFunction->AddPoint(255, 0.2);

  // Create transfer mapping scalar value to color
  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(80.0,0.0,0.0,0.0);
  colorTransferFunction->AddRGBPoint(120.0,0.0,0.0,1.0);
  colorTransferFunction->AddRGBPoint(160.0,1.0,0.0,0.0);
  colorTransferFunction->AddRGBPoint(200.0,0.0,1.0,0.0);
  colorTransferFunction->AddRGBPoint(255.0,0.0,1.0,1.0);

  // The property describes how the data will look
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(colorTransferFunction.GetPointer());
  volumeProperty->SetScalarOpacity(opacityTransferFunction.GetPointer());
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper / ray cast function know how to render the data
  vtkNew<vtkUnstructuredGridVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // contour the second dataset
  vtkNew<vtkContourFilter> contour;
  contour->SetValue(0,80);
  contour->SetInputConnection(reader2->GetOutputPort());

  // create a mapper for the polygonal data
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // create an actor for the polygonal data
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> ren1;
  ren1->AddViewProp(actor.GetPointer());
  ren1->AddVolume(volume.GetPointer());

  // // Create the renderwindow, interactor and renderer
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(401, 399); // NPOT size
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());
  ren1->SetBackground(0.3, 0.3, 0.4);
  renderWindow->AddRenderer(ren1.GetPointer());

  ren1->ResetCamera();
  renderWindow->Render();

// Attach OSPRay render pass
  vtkNew<vtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren1->SetPass(osprayPass.GetPointer());
  }

  volumeMapper->DebugOn();
  int retVal;
  retVal = !( vtkTesting::InteractorEventLoop(argc, argv,
                                                iren.GetPointer(),
                                                TestOSPRayTetrahedraMapperLog));
  return !retVal;
}
