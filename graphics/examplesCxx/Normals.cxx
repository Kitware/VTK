#include "vtk.h"

main ()
{
  vtkRenderer *aren    = vtkRenderer::New();
  vtkRenderWindow *renWin  = vtkRenderWindow::New();
    renWin->AddRenderer(aren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSTLReader *stl = vtkSTLReader::New();
    stl->SetFileName("../../data/cadPart.stl");
    stl->DebugOn();

  vtkPolyDataNormals *normals = vtkPolyDataNormals::New();
    normals->SetInput(stl->GetOutput());
    normals->SetFeatureAngle(60);
    normals->DebugOn();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
    mapper->SetInput(normals->GetOutput());

  vtkActor *actor1 = vtkActor::New();
    actor1->SetMapper(mapper);
    actor1->GetProperty()->SetColor(0.8,1.0,0.9);

  aren->AddActor(actor1);
  aren->SetBackground(0.2,0.2,0.2);

  renWin->Render();

  // interact with data
  iren->Start();

  // Clean up
  aren->Delete();
  renWin->Delete();
  iren->Delete();
  stl->Delete();
  normals->Delete();
  mapper->Delete();
  actor1->Delete();
}
