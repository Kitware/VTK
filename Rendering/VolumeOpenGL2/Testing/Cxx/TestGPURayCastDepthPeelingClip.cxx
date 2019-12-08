/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDepthPeelingClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 *  Tests depth peeling pass with volume rendering + clipping.
 */

#include <vtkCallbackCommand.h>
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
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderTimerLog.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkTestingObjectFactory.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include <cassert>

namespace
{

void RenderComplete(vtkObject* obj, unsigned long, void*, void*)
{
  vtkRenderWindow* renWin = vtkRenderWindow::SafeDownCast(obj);
  assert(renWin);

  vtkRenderTimerLog* timer = renWin->GetRenderTimer();
  while (timer->FrameReady())
  {
    std::cout << "-- Frame Timing:------------------------------------------\n";
    timer->PopFirstReadyFrame().Print(std::cout);
    std::cout << "\n";
  }
}

} // end anon namespace

class SamplingDistanceCallback : public vtkCommand
{
public:
  static SamplingDistanceCallback* New() { return new SamplingDistanceCallback; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* vtkNotUsed(data)) override
  {
    switch (event)
    {
      case vtkCommand::StartInteractionEvent:
      {
        // Higher ImageSampleDistance to make the volume-rendered image's
        // resolution visibly lower during interaction.
        this->Mapper->SetImageSampleDistance(6.5);
      }
      break;

      case vtkCommand::EndInteractionEvent:
      {
        // Default ImageSampleDistance
        this->Mapper->SetImageSampleDistance(1.0);
      }
    }
  }

  vtkGPUVolumeRayCastMapper* Mapper = nullptr;
};

int TestGPURayCastDepthPeelingClip(int argc, char* argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> ren;
  renWin->Render(); // Create the context
  renWin->AddRenderer(ren);
  vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(ren);
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

  // Setup the rendertimer observer:
  vtkNew<vtkCallbackCommand> renderCompleteCB;
  renderCompleteCB->SetCallback(RenderComplete);
  renWin->GetRenderTimer()->LoggingEnabledOn();
  renWin->AddObserver(vtkCommand::EndEvent, renderCompleteCB);

  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // Add outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  // Test clipping now
  const double* bounds = reader->GetOutput()->GetBounds();
  vtkNew<vtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane1->SetNormal(0.8, 0.0, 0.0);

  vtkNew<vtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.35 * (bounds[2] + bounds[3]), 0.0);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  vtkNew<vtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  volumeMapper->SetClippingPlanes(clipPlaneCollection);

  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);
  ren->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.6, 0.6);

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  int dims[3];
  double spacing[3], center[3], origin[3];
  reader->Update();
  vtkSmartPointer<vtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  // Add sphere 1
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
  sphereActor->SetMapper(sphereMapper);

  // Add sphere 2
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
  sphereActor2->SetMapper(sphereMapper2);

  // Add actors
  ren->AddVolume(volume);
  ren->AddActor(outlineActor);
  ren->AddActor(sphereActor);
  ren->AddActor(sphereActor2);

  // Configure depth peeling
  ren->SetUseDepthPeeling(1);
  ren->SetOcclusionRatio(0.0);
  ren->SetMaximumNumberOfPeels(17);
  ren->SetUseDepthPeelingForVolumes(true);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style);

  vtkNew<SamplingDistanceCallback> callback;
  callback->Mapper = volumeMapper;
  style->AddObserver(vtkCommand::StartInteractionEvent, callback);
  style->AddObserver(vtkCommand::EndInteractionEvent, callback);

  ren->ResetCamera();
  renWin->Render();

  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
