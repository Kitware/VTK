#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkQuadric.h"
#include "vtkSampleFunction.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *aren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
//
// Create surfaces F(x,y,z) = constant
//
  // Sample quadric function
  vtkQuadric *quadric = vtkQuadric::New();
      quadric->SetCoefficients(.5,1,.2,0,.1,0,0,.2,0,0);
  vtkSampleFunction *sample = vtkSampleFunction::New();
      sample->SetSampleDimensions(50,50,50);
      sample->SetImplicitFunction(quadric);
  vtkContourFilter *contour = vtkContourFilter::New();
      contour->SetInput(sample->GetOutput());
      contour->GenerateValues(5,0,1.2);
  vtkPolyDataMapper *contourMapper = vtkPolyDataMapper::New();
      contourMapper->SetInput(contour->GetOutput());
      contourMapper->SetScalarRange(0,1.2);
  vtkActor *contourActor = vtkActor::New();
      contourActor->SetMapper(contourMapper);

  // Create outline
  vtkOutlineFilter *outline = vtkOutlineFilter::New();
      outline->SetInput(sample->GetOutput());
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
      outlineMapper->SetInput(outline->GetOutput());
  vtkActor *outlineActor = vtkActor::New();
      outlineActor->SetMapper(outlineMapper);
      outlineActor->GetProperty()->SetColor(0,0,0);
//
// Rendering stuff
//
  aren->SetBackground(1,1,1);
      aren->AddActor(contourActor);
      aren->AddActor(outlineActor);

  renWin->SetSize(300,300);
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
}
