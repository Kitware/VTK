#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkShrinkFilter.h"
#include "vtkElevationFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *renderer = vtkRenderer::New();
  renderer->GetCullers()->RemoveAllItems();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(12); sphere->SetPhiResolution(12);

  vtkShrinkFilter *shrink = vtkShrinkFilter::New();
    shrink->SetInput(sphere->GetOutput());
    shrink->SetShrinkFactor(0.9);

  vtkElevationFilter *colorIt = vtkElevationFilter::New();
    colorIt->SetInput(shrink->GetOutput());
    colorIt->SetLowPoint(0,0,-.5);
    colorIt->SetHighPoint(0,0,.5);

  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
    mapper->SetInput(colorIt->GetOutput());

  vtkActor *actor = vtkActor::New();
    actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);
  
  renWin->Render(); // execute first time
  shrink->SetInput(colorIt->GetOutput()); // create loop
  renWin->Render(); // begin looping
  renWin->Render();
  renWin->Render();

  SAVEIMAGE( renWin );

  // interact with data
  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  shrink->Delete();
  colorIt->Delete();
  mapper->Delete();
  actor->Delete();
}
