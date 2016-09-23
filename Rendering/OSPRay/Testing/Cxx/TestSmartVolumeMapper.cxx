/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the smart volume mapper and composite method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method.

#include <vtkCamera.h>
#include <vtkClipPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkStructuredPointsReader.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkOSPRayPass.h>
#include <vtkProperty.h>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOSPRay);

int TestSmartVolumeMapper(int argc, char *argv[])
{
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }
  double scalarRange[2];

  vtkNew<vtkActor> dssActor;
  vtkNew<vtkPolyDataMapper> dssMapper;
  vtkNew<vtkSmartVolumeMapper> volumeMapper;
  if (useOSP)
  {
    volumeMapper->SetRequestedRenderModeToOSPRay();
  }

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());
#ifdef VTK_OPENGL2
  volumeMapper->SetSampleDistance(0.01);
#endif

  // Put inside an open box to evaluate composite order
  vtkNew<vtkDataSetSurfaceFilter> dssFilter;
  dssFilter->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkClipPolyData> clip;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0,50,0);
  plane->SetNormal(0,-1,0);
  clip->SetInputConnection(dssFilter->GetOutputPort());
  clip->SetClipFunction(plane.GetPointer());
  dssMapper->SetInputConnection(clip->GetOutputPort());
  dssMapper->ScalarVisibilityOff();
  dssActor->SetMapper(dssMapper.GetPointer());
  vtkProperty* property = dssActor->GetProperty();
  property->SetDiffuseColor(0.5, 0.5, 0.5);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
#ifdef VTK_OPENGL2
  volumeMapper->SetAutoAdjustSampleDistances(1);
#endif
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
//  vtkNew<vtkInteractorStyleTrackballCamera> style;
//  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.1);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.8, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.0, 0.8, 0.1);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddViewProp(volume.GetPointer());
  ren->AddActor(dssActor.GetPointer());
  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
