#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkQuadric.h"
#include "vtkSampleFunction.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkLight.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkCamera *camera;
  float range[2];
  
  vtkRenderer *aren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
  renWin->SetSize( 300, 300 );

//
// Create surface of implicit function
//
  // Sample quadric function
  vtkQuadric *quadric = vtkQuadric::New();
  quadric->SetCoefficients(1,2,3,0,1,0,0,0,0,0);

  vtkSampleFunction *sample = vtkSampleFunction::New();
    sample->SetSampleDimensions(25,25,25);
    sample->SetImplicitFunction(quadric);
    //sample->DebugOn();

  // Generate implicit surface
  vtkContourFilter *contour = vtkContourFilter::New();
    contour->SetInput(sample->GetOutput());
    range[0] = 1.0; range[1] = 6.0;
    contour->GenerateValues(3,range);
    //contour->DebugOn();

  // Map contour
  vtkPolyDataMapper *contourMapper = vtkPolyDataMapper::New();
    contourMapper->SetInput(contour->GetOutput());
    contourMapper->SetScalarRange(0,7);

  vtkActor *contourActor = vtkActor::New();
    contourActor->SetMapper(contourMapper);
//
// Create outline around data
//
  vtkOutlineFilter *outline = vtkOutlineFilter::New();
    outline->SetInput(sample->GetOutput());

  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  vtkActor *outlineActor = vtkActor::New();
    outlineActor->SetMapper(outlineMapper);
    outlineActor->GetProperty()->SetColor(1,1,1);

  vtkLight *light = vtkLight::New();
  aren->AddLight(light);
  aren->AddActor(contourActor);
  aren->AddActor(outlineActor);

  renWin->Render(); // will automatically create camera
  camera = aren->GetActiveCamera();
  light->SetFocalPoint(camera->GetFocalPoint());
  light->SetPosition(camera->GetPosition());

  renWin->Render();

  SAVEIMAGE( renWin );

  // interact with data
  iren->Start();

  // Clean up
  aren->Delete();
  renWin->Delete();
  iren->Delete();
  quadric->Delete();
  sample->Delete();
  contour->Delete();
  contourMapper->Delete();
  contourActor->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  light->Delete();
}
