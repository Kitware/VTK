#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  float i;

  // create the rendering objects
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // create the pipline, ball and spikes
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(7); sphere->SetPhiResolution(7);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);
  vtkActor *sphereActor2 = vtkActor::New();
    sphereActor2->SetMapper(sphereMapper);
  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(5);

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
  vtkActor *spikeActor2 = vtkActor::New();
    spikeActor2->SetMapper(spikeMapper);

  spikeActor->SetPosition(0,0.7,0);
  sphereActor->SetPosition(0,0.7,0);
  spikeActor2->SetPosition(0,-0.7,0);
  sphereActor2->SetPosition(0,-0.7,0);

  ren1->AddActor(sphereActor);
  ren1->AddActor(spikeActor);
  ren1->AddActor(sphereActor2);
  ren1->AddActor(spikeActor2);
  ren1->SetBackground(0.1,0.2,0.4);
  renWin->SetSize(300,300);
  renWin->DoubleBufferOn();

  // do the first render and then zoom in a little
  renWin->Render();
  ren1->GetActiveCamera()->Zoom(1.5);

  renWin->SetSubFrames(21);

  for (i = 0; i <= 1.0; i = i + 0.05)
    {
    spikeActor2->RotateY(2);
    sphereActor2->RotateY(2);
    renWin->Render();
    }

  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  sphereActor2->Delete();
  cone->Delete();
  glyph->Delete();
  spikeMapper->Delete();
  spikeActor->Delete();
  spikeActor2->Delete();
}
