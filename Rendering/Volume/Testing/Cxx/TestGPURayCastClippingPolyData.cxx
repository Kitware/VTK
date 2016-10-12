/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumePolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers additive method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
#include <vtkInteractorStyleTrackballCamera.h>


/// Tests volume clipping when intermixed with geometry.
int TestGPURayCastClippingPolyData(int argc, char *argv[])
{
  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  /// Outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper.GetPointer());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  renWin->SetSize(400, 400);
  ren->SetBackground(0.2, 0.2, 0.5);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  /// Sphere
  int dims[3];
  double spacing[3], sphereCenter[3], origin[3];
  reader->Update();
  vtkSmartPointer<vtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  sphereCenter[0] = origin[0] + spacing[0]*dims[0]/2.5;
  sphereCenter[1] = origin[1] + spacing[1]*dims[1]/2.5;
  sphereCenter[2] = origin[2] + spacing[2]*dims[2]/2.775;

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(sphereCenter);
  sphereSource->SetRadius(dims[1]/4.0);
  sphereSource->SetPhiResolution(40);
  sphereSource->SetThetaResolution(40);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  vtkNew<vtkActor> sphereActor;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper.GetPointer());

  ren->AddViewProp(volume.GetPointer());
  ren->AddActor(outlineActor.GetPointer());
  ren->AddActor(sphereActor.GetPointer());

  /// Clipping planes
  vtkNew<vtkPlane> clipPlane1;
  clipPlane1->SetOrigin(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
  clipPlane1->SetNormal(1.0, 0.0, 0.0);
  vtkNew<vtkPlane> clipPlane2;
  clipPlane2->SetOrigin(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  vtkNew<vtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1.GetPointer());
  clipPlaneCollection->AddItem(clipPlane2.GetPointer());
  volumeMapper->SetClippingPlanes(clipPlaneCollection.GetPointer());

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-30);
  ren->GetActiveCamera()->Elevation(25);
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
