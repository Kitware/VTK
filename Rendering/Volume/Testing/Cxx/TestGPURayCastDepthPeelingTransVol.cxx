/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDepthPeelingTransVol.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 *  Tests depth peeling pass with volume rendering.
 */

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTestingObjectFactory.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkProperty.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>


int TestGPURayCastDepthPeelingTransVol(int argc, char *argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkRenderer> ren;
  renWin->Render(); // Create the context
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

  double scalarRange[2];

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  renWin->SetMultiSamples(0);
  renWin->SetSize(800, 400);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.06);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.9, 0.4, 0.9);

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  int dims[3];
  double spacing[3], center[3], origin[3];
  reader->Update();
  vtkSmartPointer<vtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  // sphere 1
  center[0] = origin[0] + spacing[0] * dims[0] / 2.0;
  center[1] = origin[1] + spacing[1] * dims[1] / 2.0;
  center[2] = origin[2] + spacing[2] * dims[2] / 2.0;

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(center);
  sphereSource->SetRadius(dims[1] / 3.0);
  vtkNew<vtkActor> sphereActor;
  vtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(0.5, 0.9, 0.7);
  sphereProperty->SetOpacity(0.3);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper.GetPointer());

  // sphere 2
  center[0] += 15.0;
  center[1] += 15.0;
  center[2] += 15.0;

  vtkNew<vtkSphereSource> sphereSource2;
  sphereSource2->SetCenter(center);
  sphereSource2->SetRadius(dims[1] / 3.0);
  vtkNew<vtkActor> sphereActor2;
  sphereProperty = sphereActor2->GetProperty();
  sphereProperty->SetColor(0.9, 0.4, 0.1);
  sphereProperty->SetOpacity(0.3);
  vtkNew<vtkPolyDataMapper> sphereMapper2;
  sphereMapper2->SetInputConnection(sphereSource2->GetOutputPort());
  sphereActor2->SetMapper(sphereMapper2.GetPointer());

  // sphere 3 (transparent)
  vtkNew<vtkActor> sphereActorTransp;
  vtkProperty* sphereProperty3 = sphereActorTransp->GetProperty();
  sphereProperty3->SetColor(0.0, 1.0, 0.0);
  sphereProperty3->SetOpacity(0.0);
  vtkNew<vtkPolyDataMapper> sphereMapperTransp;
  sphereMapperTransp->SetInputConnection(sphereSource->GetOutputPort());
  sphereActorTransp->SetMapper(sphereMapperTransp.GetPointer());

  // Translucent spheres
  ren->SetBackground(1.0, 1.0, 1.0);
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);

  ren->AddVolume(volume.GetPointer());
  ren->AddActor(sphereActor.GetPointer());
  ren->AddActor(sphereActor2.GetPointer());

  ren->SetUseDepthPeeling(1);
  ren->SetOcclusionRatio(0.0);
  ren->SetMaximumNumberOfPeels(17);
  ren->SetUseDepthPeelingForVolumes(true);

  // Fully trasnparent sphere
  vtkNew<vtkRenderer> ren2;
  ren2->SetBackground(1.0, 1.0, 1.0);
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  ren2->SetActiveCamera(ren->GetActiveCamera());

  ren2->AddVolume(volume.GetPointer());
  ren2->AddActor(sphereActorTransp.GetPointer());

  ren2->SetUseDepthPeeling(1);
  ren2->SetOcclusionRatio(0.0);
  ren2->SetMaximumNumberOfPeels(17);
  ren2->SetUseDepthPeelingForVolumes(true);
  renWin->AddRenderer(ren2.GetPointer());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style.GetPointer());

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-55);
  ren->GetActiveCamera()->Elevation(35);
  ren->GetActiveCamera()->OrthogonalizeViewUp();
  renWin->Render();

  iren->Initialize();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
