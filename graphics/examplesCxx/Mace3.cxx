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
//  sphereActor->GetProperty()->SetColor(0.8,0.8,0.8);

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
//  spikeActor->GetProperty()->SetColor(0.8,0.8,0.8);

  ren1->AddActor(sphereActor);
  ren1->AddActor(spikeActor);
  ren1->SetBackground(0.2,0.3,0.4);
  renWin->SetSize(300,300);

  // do the first render and then zoom in a little
  renWin->Render();
  ren1->GetActiveCamera()->Zoom(1.4);
  renWin->SetFileName("test.ppm");
  renWin->StereoRenderOn();
  renWin->SetStereoTypeToRedBlue();
  renWin->Render();
  renWin->Render();
  renWin->SaveImageAsPPM();

  SAVEIMAGE( renWin );

  // Clean up
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  cone->Delete();
  glyph->Delete();
  spikeMapper->Delete();
  spikeActor->Delete();
}
