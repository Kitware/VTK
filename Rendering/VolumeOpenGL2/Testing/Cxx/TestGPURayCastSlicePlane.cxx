#include <vtkColorTransferFunction.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

//----------------------------------------------------------------------------
int TestGPURayCastSlicePlane(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRTAnalyticSource> data;
  data->SetWholeExtent(-100, 100, -100, 100, -100, 100);
  data->Update();

  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(data->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.5);
  mapper->SetBlendModeToSlice();

  // we also test if slicing works with cropping
  mapper->SetCroppingRegionPlanes(0, 100, -100, 100, -100, 100);
  mapper->CroppingOn();

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(150.0, 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(190.0, 0.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(220.0, 1.0);
  scalarOpacity->AddPoint(150.0, 0.2);
  scalarOpacity->AddPoint(190.0, 0.6);

  vtkNew<vtkPlane> slice;
  slice->SetOrigin(1.0, 0.0, 0.0);
  slice->SetNormal(0.707107, 0.0, 0.707107);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);
  volumeProperty->SetSliceFunction(slice);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->SetInteractorStyle(style);

  renderWindow->Render();
  renderWindowInteractor->Start();

  return 0;
}
