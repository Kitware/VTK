#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkRenderWindowInteractor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderer *ren2 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);
    renWin->AddRenderer(ren2);

  vtkRenderer *ren3 = vtkRenderer::New();
  vtkRenderWindow *renWin2 = vtkRenderWindow::New();
    renWin2->AddRenderer(ren3);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor1 = vtkActor::New();
    sphereActor1->SetMapper(sphereMapper);
  vtkActor *sphereActor2 = vtkActor::New();
    sphereActor2->SetMapper(sphereMapper);
  vtkActor *sphereActor3 = vtkActor::New();
    sphereActor3->SetMapper(sphereMapper);

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

  vtkActor *spikeActor1 = vtkActor::New();
    spikeActor1->SetMapper(spikeMapper);
  vtkActor *spikeActor2 = vtkActor::New();
    spikeActor2->SetMapper(spikeMapper);
  vtkActor *spikeActor3 = vtkActor::New();
    spikeActor3->SetMapper(spikeMapper);

  ren1->AddActor(sphereActor1);
  ren1->AddActor(spikeActor1);
  ren1->SetBackground(0.4,0.1,0.2);
  ren1->SetViewport(0,0,0.5,1.0);

  ren2->AddActor(sphereActor2);
  ren2->AddActor(spikeActor2);
  ren2->SetBackground(0.1,0.2,0.4);
  ren2->SetViewport(0.5,0,1.0,1.0);
  renWin->SetSize(300,150);
  renWin->SetPosition(0, 400);

  ren3->AddActor(sphereActor3);
  ren3->AddActor(spikeActor3);
  ren3->SetBackground(0.1,0.4,0.2);
  renWin2->SetSize(300,300);
  renWin2->SetPosition(0, 50);

  // allow keyboard manipulation of object
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
  renWin->Render();

  vtkRenderWindowInteractor *iren2 = vtkRenderWindowInteractor::New();
    iren2->SetRenderWindow(renWin2);
  renWin2->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  ren1->Delete();
  ren2->Delete();
  renWin->Delete();
  ren3->Delete();
  renWin2->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor1->Delete();
  sphereActor2->Delete();
  sphereActor3->Delete();
  cone->Delete();
  glyph->Delete();
  spikeMapper->Delete();
  spikeActor1->Delete();
  spikeActor2->Delete();
  spikeActor3->Delete();
  iren->Delete();
  iren2->Delete();
}


