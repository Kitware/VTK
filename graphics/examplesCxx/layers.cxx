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

  // Set the render window's layers, partitioning the z-buffer.
  // It is already known that layer 0 starts at 0. and layer 2 ends at 1.,
  // so we just need to send in where layer 0 stops/layer 1 begins and
  // where layer 1 stops/layer 2 begins.  Layer 2 is very thin since 
  // it is just used for 2D actors.  You can have the render window partition
  // the z-buffer for you by calling the same function with only the number
  // of layers.
  int numLayers = 3;
  float layers[2] = { 0.5, 0.99 };
  renWin->SetLayers(numLayers, layers);

  // Create the background.
  vtkRenderer *background = vtkRenderer::New();
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
  
  renWin->AddRenderer(background);
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


