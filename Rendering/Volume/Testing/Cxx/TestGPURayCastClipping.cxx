/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastClipping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers cropping on volume datasets.

#include <vtkActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
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

int TestGPURayCastClipping(int argc, char *argv[])
{
  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");

  reader->SetFileName(volumeFile);
  reader->Update();
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // Add outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper.GetPointer());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  // Testing prefers image comparison with small images
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  // Intentional odd and NPOT  width/height
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  // Test cropping now
  double* bounds = reader->GetOutput()->GetBounds();
  vtkNew<vtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane1->SetNormal(1.0, 0.0, 0.0);

  vtkNew<vtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.55 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane2->SetNormal(-1.0, 0.0, 0.0);

  vtkNew<vtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1.GetPointer());
  clipPlaneCollection->AddItem(clipPlane2.GetPointer());
  volumeMapper->SetClippingPlanes(clipPlaneCollection.GetPointer());

  // Setup volume actor
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddViewProp(volume.GetPointer());
  ren->AddActor(outlineActor.GetPointer());
  ren->ResetCamera();
  renWin->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
