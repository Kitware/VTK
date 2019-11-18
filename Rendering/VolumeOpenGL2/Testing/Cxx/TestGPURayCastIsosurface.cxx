#include <vtkColorTransferFunction.h>
#include <vtkContourValues.h>
#include <vtkDataArray.h>
#include <vtkFloatingPointExceptions.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

//----------------------------------------------------------------------------
int TestGPURayCastIsosurface(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{

  vtkFloatingPointExceptions::Disable();
  vtkNew<vtkRTAnalyticSource> data;
  data->SetWholeExtent(-100, 100, -100, 100, -100, 100);
  data->Update();
  std::cout << "range: " << data->GetOutput()->GetPointData()->GetScalars()->GetRange()[0] << ", "
            << data->GetOutput()->GetPointData()->GetScalars()->GetRange()[1] << std::endl;
  ;
  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(data->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.5);
  mapper->SetBlendModeToIsoSurface();

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(150.0, 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(190.0, 0.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(220.0, 1.0);
  scalarOpacity->AddPoint(150.0, 0.2);
  scalarOpacity->AddPoint(190.0, 0.6);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->SetInteractorStyle(style);

  // Check that no errors is created when no contour values are set
  renderWindow->Render();
  volumeProperty->GetIsoSurfaceValues()->SetValue(0, 220.0);
  renderWindow->Render();
  volumeProperty->GetIsoSurfaceValues()->SetNumberOfContours(0);
  renderWindow->Render();

  // Now add some contour values to draw iso surfaces
  volumeProperty->GetIsoSurfaceValues()->SetValue(0, 220.0);
  volumeProperty->GetIsoSurfaceValues()->SetValue(1, 150.0);
  volumeProperty->GetIsoSurfaceValues()->SetValue(2, 190.0);

  renderWindow->Render();

  renderWindowInteractor->Start();

  return 0;
}
