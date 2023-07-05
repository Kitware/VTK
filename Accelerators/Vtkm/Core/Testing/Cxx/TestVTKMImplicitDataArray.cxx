// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkDataSetMapper.h>
#include <vtkGenericImageInterpolator.h>
#include <vtkImageData.h>
#include <vtkImagePlaneWidget.h>
#include <vtkImageReslice.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <vtkmDataArray.h>

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <array>
#include <functional>

namespace
{

struct TransformFnctr
{
  VTKM_EXEC_CONT
  double operator()(const vtkm::Vec3f& point) const { return vtkm::Magnitude(point); }
};

} // namespace

//------------------------------------------------------------------------------
int TestVTKMImplicitDataArray(int, char*[])
{
  vtkm::Id dimension = 10;
  vtkm::Vec3f boundsMin(0.0f);
  vtkm::Vec3f boundsMax(3.0f, 3.0f, 2.0f);
  vtkm::Id3 dim3(dimension);
  vtkm::Vec3f origin = boundsMin;
  vtkm::Vec3f spacing =
    (boundsMax - boundsMin) / vtkm::Vec3f(static_cast<vtkm::FloatDefault>(dimension));

  vtkNew<vtkImageData> imageData;
  imageData->SetDimensions(dim3[0], dim3[1], dim3[2]);
  imageData->SetSpacing(spacing[0], spacing[1], spacing[2]);
  imageData->SetOrigin(origin[0], origin[1], origin[2]);

  vtkNew<vtkmDataArray<double>> array;
  array->SetVtkmArrayHandle(vtkm::cont::make_ArrayHandleTransform(
    vtkm::cont::ArrayHandleUniformPointCoordinates(dim3, origin, spacing), TransformFnctr{}));
  imageData->GetPointData()->SetScalars(array);

  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkImagePlaneWidget> planeWidgetZ;

  planeWidgetZ->GetReslice()->SetInterpolator(vtkNew<vtkGenericImageInterpolator>());
  vtkGenericImageInterpolator::SafeDownCast(planeWidgetZ->GetReslice()->GetInterpolator())
    ->SetInterpolationModeToLinear();
  planeWidgetZ->GetReslice()->GetInterpolator()->Update();
  planeWidgetZ->SetInteractor(renderWindowInteractor);
  planeWidgetZ->SetKeyPressActivationValue('x');
  planeWidgetZ->SetUseContinuousCursor(true);
  planeWidgetZ->GetPlaneProperty()->SetColor(1, 0, 0);
  planeWidgetZ->SetInputData(imageData);
  planeWidgetZ->SetPlaneOrientationToZAxes();
  planeWidgetZ->SetSliceIndex(1);
  planeWidgetZ->DisplayTextOn();
  planeWidgetZ->On();
  planeWidgetZ->InteractionOff();
  planeWidgetZ->InteractionOn();

  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
