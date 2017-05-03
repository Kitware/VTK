/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeUpdate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * This test volume tests whether updating the volume MTime updates the ,
 * geometry in the volume mapper.
 *
 * Added renderer to expand coverage for vtkDualDepthPeelingPass.
 */

#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointDataToCellData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTesting.h>
#include <vtkTestingObjectFactory.h>
#include <vtkTestUtilities.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>


int TestGPURayCastCellData(int argc, char *argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  renWin->Render(); // Create the context

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  vtkOpenGLRenderer *oglRen = vtkOpenGLRenderer::SafeDownCast(ren.Get());
  assert(oglRen); // This test should only be enabled for OGL2 backend.
  // This will print details about why depth peeling is unsupported:
  oglRen->SetDebug(1);
  bool supported = oglRen->IsDualDepthPeelingSupported();
  oglRen->SetDebug(0);
  if (!supported)
  {
    std::cerr << "Skipping test; volume peeling not supported.\n";
    return VTK_SKIP_RETURN_CODE;
  }

  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  vtkNew<vtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetInputConnection(pointToCell->GetOutputPort());

  // Add outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(pointToCell->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper.GetPointer());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  renWin->SetMultiSamples(0);
  renWin->SetSize(800, 400);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  // Initialize OpenGL context
  renWin->Render();

  // Renderer without translucent geometry
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);
  ren->SetBackground(0.2, 0.2, 0.5);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);
  volumeProperty->SetColor(colorTransferFunction.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->AddActor(outlineActor.GetPointer());

  // Renderer with translucent geometry
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(80.0, 60.0, 30.0);
  sphereSource->SetRadius(30.0);

  vtkNew<vtkActor> sphereActor;
  vtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(1., 0.9, 1);
  sphereProperty->SetOpacity(0.4);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper.GetPointer());

  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  ren2->SetBackground(0.2, 0.2, 0.5);
  ren2->SetActiveCamera(ren->GetActiveCamera());

  ren2->SetUseDepthPeeling(1);
  ren2->SetOcclusionRatio(0.0);
  ren2->SetMaximumNumberOfPeels(5);
  ren2->SetUseDepthPeelingForVolumes(true);

  ren2->AddVolume(volume.GetPointer());
  ren2->AddActor(outlineActor.GetPointer());
  ren2->AddActor(sphereActor.GetPointer());
  renWin->AddRenderer(ren2.GetPointer());

  ren->ResetCamera();

  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
