#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkClipPolyData.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// Generate a sphere. Create a view frustum looking at the sphere
// Clip anything inside the frustum, then back away and view result

int main( int argc, char *argv[] )
{
  vtkDebugLeaks::PromptUserOff();

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(36);
    sphere->SetPhiResolution(18);
    sphere->SetRadius(1);

  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInput( sphere->GetOutput());

  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(400,300);

  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetPosition(1.5, 0.0, 0.0);
  renderer->ResetCameraClippingRange();

  // Display once with camera in position 1
  // Ensures clipping planes are initialized (camera matrix really).
  renWin->Render();

  // Now get the camera frustum and then move the camera away to see the
  // clipped away stuff
  float aspect=400.0/300.0, planeequations[24];
  camera->GetFrustumPlanes(aspect, planeequations);

  vtkPlanes *implictplanes = vtkPlanes::New();
  implictplanes->SetFrustumPlanes(planeequations);

  vtkClipPolyData *clipper = vtkClipPolyData::New();
  clipper->SetInput(sphere->GetOutput());
  clipper->SetClipFunction(implictplanes);
  clipper->SetGenerateClipScalars(1);
  clipper->SetInsideOut(0);
  sphereMapper->SetInput( clipper->GetOutput());

  camera->SetPosition(-4.0, 0.25, 0.25);
  renderer->ResetCameraClippingRange();

  sphereActor->GetProperty()->SetColor(0.0,0.0,0.0);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  implictplanes->Delete();
  clipper->Delete();

  return !retVal;
}
