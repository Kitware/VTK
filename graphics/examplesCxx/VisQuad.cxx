#include "vtk.h"

main ()
{
  vtkRenderWindow *renWin;
  vtkRenderer *ren1;
  vtkRenderWindowInteractor *iren;
  vtkActor *outlineActor;
  vtkActor *contActor;
  vtkPolyMapper *outlineMapper, *contMapper;
  vtkSampleFunction *sample;
  vtkQuadric *quadric;
  vtkOutlineFilter *outline;
  vtkContourFilter *contours;

  ren1 = vtkRenderer::New();
  renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  
  // Quadric definition
  quadric = vtkQuadric::New();
    quadric->SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0);

  sample = vtkSampleFunction::New();
    sample->SetSampleDimensions(50,50,50);
    sample->SetImplicitFunction(quadric);

  // Create five surfaces F(x,y,z) = constant between range specified
  contours = vtkContourFilter::New();
    contours->SetInput(sample->GetOutput());
    contours->GenerateValues(5, 0.0, 1.2);

  contMapper = vtkPolyMapper::New();
    contMapper->SetInput(contours->GetOutput());
    contMapper->SetScalarRange(0.0, 1.2);

  contActor = vtkActor::New();
    contActor->SetMapper(contMapper);

  // Create outline
  outline = vtkOutlineFilter::New();
    outline->SetInput(sample->GetOutput());

  outlineMapper = vtkPolyMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  outlineActor = vtkActor::New();
    outlineActor->SetMapper(outlineMapper);
    outlineActor->GetProperty()->SetColor(0,0,0);

  ren1->SetBackground(1,1,1);
  ren1->AddActor(contActor);
  ren1->AddActor(outlineActor);

  renWin->Render();

  // interact with data
  iren->Start();
}
