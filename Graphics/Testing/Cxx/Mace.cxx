#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

int main( int argc, char *argv[] )
{
  vtkDebugLeaks::PromptUserOff();

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);

  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(6);

  vtkGlyph3D *glyph = vtkGlyph3D::New();
    glyph->SetInput(sphere->GetOutput());
    glyph->SetSource(cone->GetOutput());
    glyph->SetVectorModeToUseNormal();
    glyph->SetScaleModeToScaleByVector();
    glyph->SetScaleFactor(0.25);

  vtkPolyDataMapper *spikeMapper = vtkPolyDataMapper::New();
    spikeMapper->SetInput(glyph->GetOutput());

  vtkActor *spikeActor = vtkActor::New();
    spikeActor->SetMapper(spikeMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(spikeActor);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  cone->Delete();
  glyph->Delete();
  spikeMapper->Delete();
  spikeActor->Delete();

  return !retVal;
}
