/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastLabelMap1Label.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Labeled data volume rendering with a single label
 */

#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkMetaImageWriter.h>
#include <vtkObjectFactory.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSampleFunction.h>
#include <vtkSmartPointer.h>
#include <vtkSphere.h>
#include <vtkTesting.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

namespace TestGPURayCastLabelMap1LabelNS
{
void CreateImageData(vtkImageData* a_image)
{
  // Create a spherical implicit function.
  vtkSmartPointer<vtkSphere> sphere = vtkSmartPointer<vtkSphere>::New();
  sphere->SetRadius(0.1);
  sphere->SetCenter(0.0, 0.0, 0.0);

  vtkSmartPointer<vtkSampleFunction> sampleFunc = vtkSmartPointer<vtkSampleFunction>::New();
  sampleFunc->SetImplicitFunction(sphere);
  sampleFunc->SetOutputScalarTypeToDouble();
  sampleFunc->SetSampleDimensions(127, 127, 127);
  sampleFunc->SetModelBounds(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  sampleFunc->SetCapping(false);
  sampleFunc->SetComputeNormals(false);
  sampleFunc->SetScalarArrayName("values");
  sampleFunc->Update();

  vtkDataArray* scalars = sampleFunc->GetOutput()->GetPointData()->GetScalars("values");
  double scalarRange[2];
  scalars->GetRange(scalarRange);

  vtkSmartPointer<vtkImageShiftScale> shiftScale = vtkSmartPointer<vtkImageShiftScale>::New();
  shiftScale->SetInputConnection(sampleFunc->GetOutputPort());

  shiftScale->SetShift(-scalarRange[0]);
  double mag = scalarRange[1] - scalarRange[0];
  if (mag == 0.0)
  {
    mag = 1.0;
  }
  shiftScale->SetScale(255.0 / mag);
  shiftScale->SetOutputScalarTypeToShort();
  shiftScale->Update();

  a_image->DeepCopy(shiftScale->GetOutput());
}
} // end of namespace TestGPURayCastLabelMap1LabelNS

int TestGPURayCastLabelMap1Label(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Create a sphere volume
  vtkNew<vtkImageData> imageData;
  TestGPURayCastLabelMap1LabelNS::CreateImageData(imageData);

  // prepare the rendering pipeline
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.3, 0.3);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // setup gpu rendering
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetBlendModeToComposite();
  mapper->SetInputData(imageData);
  mapper->SetAutoAdjustSampleDistances(true);

  // Main TF
  vtkNew<vtkPiecewiseFunction> opacityFunc;
  opacityFunc->AddPoint(0.0, 0.0);
  opacityFunc->AddPoint(80.0, 1.0);
  opacityFunc->AddPoint(80.1, 0.0);
  opacityFunc->AddPoint(255.0, 0.0);

  vtkNew<vtkColorTransferFunction> colorFunc;
  colorFunc->AddRGBPoint(0.0, 1.0, 0.0, 0.0);    // RED everywhere
  colorFunc->AddRGBPoint(40.0, 1.0, 0.0, 0.0);   // RED everywhere
  colorFunc->AddRGBPoint(255.0, 1.0, 0.0, 01.0); // RED everywhere

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetShade(true);
  volumeProperty->SetIndependentComponents(true);
  volumeProperty->SetColor(colorFunc);
  volumeProperty->SetScalarOpacity(opacityFunc);
  volumeProperty->SetInterpolationTypeToLinear();

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  renderer->AddVolume(volume);
  renderer->ResetCamera();

  renWin->Render();

  // label map pipeline
  // prepare an empty label map of the same size;
  vtkNew<vtkImageData> labelMap;
  labelMap->SetOrigin(imageData->GetOrigin());
  labelMap->SetSpacing(imageData->GetSpacing());
  labelMap->SetDimensions(imageData->GetDimensions());
  labelMap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  memset(labelMap->GetScalarPointer(), 1, sizeof(unsigned char) * labelMap->GetNumberOfPoints());

  vtkNew<vtkColorTransferFunction> labelMapColorFunc;
  labelMapColorFunc->AddRGBPoint(0.0, 0.0, 1.0, 0.0);   // GREEN everywhere
  labelMapColorFunc->AddRGBPoint(40.0, 0.0, 1.0, 0.0);  // GREEN everywhere
  labelMapColorFunc->AddRGBPoint(255.0, 0.0, 1.0, 0.0); // GREEN everywhere

  volumeProperty->SetLabelColor(1, labelMapColorFunc);
  volumeProperty->SetLabelScalarOpacity(1, opacityFunc);

  mapper->SetMaskInput(labelMap);

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
