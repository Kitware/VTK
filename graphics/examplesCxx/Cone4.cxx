#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  // create a rendering window and renderer
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
    renWindow->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWindow);
  renWindow->SetSize( 300, 300 );

  // create an actor and give it cone geometry
  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(8);
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    coneMapper->SetInput(cone->GetOutput());
  vtkActor *cone1 = vtkActor::New();
    cone1->SetMapper(coneMapper);
    cone1->GetProperty()->SetColor(0.2000,0.6300,0.7900);
    cone1->GetProperty()->SetDiffuse(0.7);
    cone1->GetProperty()->SetSpecular(0.4);
    cone1->GetProperty()->SetSpecularPower(20);

  vtkProperty *prop = vtkProperty::New();
    prop->SetColor(1.0000, 0.3882, 0.2784);
    prop->SetDiffuse(0.7);
    prop->SetSpecular(0.4);
    prop->SetSpecularPower(20);

  vtkActor *cone2 = vtkActor::New();
    cone2->SetMapper(coneMapper);
    cone2->SetProperty(prop);
    cone2->SetPosition(0,2,0);

  // assign our actor to the renderer
  ren->AddActor(cone1);
  ren->AddActor(cone2);

  // draw the resulting scene
  renWindow->Render();

  SAVEIMAGE( renWindow );

  //  Begin mouse interaction
  iren->Start();

  // Clean up
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  cone->Delete();
  coneMapper->Delete();
  cone1->Delete();
  prop->Delete();
  cone2->Delete();
}

