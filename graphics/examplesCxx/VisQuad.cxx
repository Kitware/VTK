#include "vtk.h"

main ()
{
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
  
  // Quadric definition
  vtkQuadric *quadric = vtkQuadric::New();
    quadric->SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0);

  vtkSampleFunction *sample = vtkSampleFunction::New();
    sample->SetSampleDimensions(30,30,30);
    sample->SetImplicitFunction(quadric);

  // Create five surfaces F(x,y,z) = constant between range specified
  vtkContourFilter *contours = vtkContourFilter::New();
    contours->SetInput(sample->GetOutput());
    contours->GenerateValues(5, 0.0, 1.2);
    contours->Update();

  vtkPolyDataMapper *contMapper = vtkPolyDataMapper::New();
    contMapper->SetInput(contours->GetOutput());
    contMapper->SetScalarRange(0.0, 1.2);

  vtkActor *contActor = vtkActor::New();
    contActor->SetMapper(contMapper);

  // Create outline
  vtkOutlineFilter *outline = vtkOutlineFilter::New();
    outline->SetInput(sample->GetOutput());

  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  vtkActor *outlineActor = vtkActor::New();
    outlineActor->SetMapper(outlineMapper);
    outlineActor->GetProperty()->SetColor(0,0,0);

  ren1->SetBackground(1,1,1);
  ren1->AddActor(contActor);
  ren1->AddActor(outlineActor);

  renWin->Render();

  // interact with data
  iren->Start();

  // Clean up
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  quadric->Delete();
  sample->Delete();
  contours->Delete();
  contMapper->Delete();
  contActor->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
}
