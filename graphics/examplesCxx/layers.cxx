#include "SaveImage.h"

#include <vtkCubeSource.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include <vtkImageGridSource.h>
#include <vtkImageActor.h>

#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>


int main(int argc, char *argv[])
{
  // Create a sphere to be used later.
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor = vtkActor::New();
  sphereActor->SetMapper(sphereMapper);

  // Create a grid image to be used later.
  vtkImageGridSource *image      = vtkImageGridSource::New();
  vtkImageActor      *imageActor = vtkImageActor::New();
  imageActor->SetInput(image->GetOutput());

  vtkRenderWindow *renWin= vtkRenderWindow::New();
  renWin->SetNumLayers(3);

  // Create the background.
  vtkRenderer *background = vtkRenderer::New();
  renWin->AddRenderer(background);
  background->SetInteractive(0);
  background->SetLayer(2);
  background->SetBackground(0., 1., 1.);
  background->AddActor2D(imageActor);
  
  // Create a middle renderer.
  vtkRenderer *ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  ren->AddActor(sphereActor);
  ren->SetLayer(1);
  ren->SetInteractive(1);
  
  // Create a checkerboard of foreground renderers.
  int i, j;
  int numSteps = 4;
  for (i = 0 ; i < numSteps ; i++)
    {
    for (j = 0 ; j < numSteps ; j++)
      {
      if ((i+j) % 2 == 1)
        {
        vtkRenderer *checker_square = vtkRenderer::New();
        renWin->AddRenderer(checker_square);
        checker_square->SetInteractive(0);
        checker_square->SetLayer(0);
        float x = ((float) i) / numSteps;
        float y = ((float) j) / numSteps;
        float step = 1. / numSteps;
        checker_square->SetViewport(x, y, x+step, y+step);
        checker_square->AddActor(sphereActor);
        }
      }
    }

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();

  SAVEIMAGE(renWin);

  iren->Start();

  // Clean up...
}


