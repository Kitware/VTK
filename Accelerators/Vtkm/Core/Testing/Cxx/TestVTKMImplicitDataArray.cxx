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

#include <vtkm/cont/ArrayHandleImplicit.h>

#include <array>
#include <functional>

//------------------------------------------------------------------------------
int TestVTKMImplicitDataArray(int, char*[])
{
  typedef std::function<double(const std::array<double, 3>&)> ScalarFunction;

  ScalarFunction function = [](const std::array<double, 3>& p) {
    return sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
  };

  std::size_t dimension = 10;

  vtkNew<vtkImageData> imageData;
  {
    std::array<double, 6> boundingBox = { 0, 3, 0, 3, 0, 2 };
    imageData->SetDimensions(
      static_cast<int>(dimension), static_cast<int>(dimension), static_cast<int>(dimension));
    imageData->SetSpacing((boundingBox[1] - boundingBox[0]) / dimension,
      (boundingBox[3] - boundingBox[2]) / dimension, (boundingBox[5] - boundingBox[4]) / dimension);
    imageData->SetOrigin(boundingBox[0], boundingBox[2], boundingBox[4]);
  }

  vtkImageData* imageDataPtr = imageData.Get();

  struct Shim
  {
    Shim() = default;
    Shim(const ScalarFunction& f, vtkImageData* i)
      : function(f)
      , imageDataPtr(i)
    {
    }
    ScalarFunction function;
    vtkImageData* imageDataPtr;
    double operator()(vtkm::Id i) const
    {
      std::array<double, 3> p;
      imageDataPtr->GetPoint(i, p.data());
      return function(p);
    }
  };

  vtkNew<vtkmDataArray<double>> array;
  array->SetVtkmArrayHandle(
    vtkm::cont::make_ArrayHandleImplicit(Shim(function, imageDataPtr), std::pow(dimension, 3)));

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
