#include "vtkRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkFloatPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkFloatPoints.h"
#include "vtkSphereSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

// Global variables to allow access by UserMethod
vtkRenderWindow *renWin;
vtkPolyData *inputDataSet;

// User Method to do animation for 10 "frames"
void UserMethod(void *arg)
{
   float p[3];
   for (int n=0; n<50; n++)
   {
      // get the current particle locations
      vtkFloatPoints *oldPoints = (vtkFloatPoints *) (inputDataSet->GetPoints());
      // Create new points by adding a random component to the old
      vtkFloatPoints *newPoints = vtkFloatPoints::New();
      for (int i=0; i<oldPoints->GetNumberOfPoints(); i++)
      {
         oldPoints->GetPoint(i, p);
         p[0] += (float) (0.1*(RAND_MAX/2 - rand())/RAND_MAX);
         p[1] += (float) (0.1*(RAND_MAX/2 - rand())/RAND_MAX);
         p[2] += (float) (0.1*(RAND_MAX/2 - rand())/RAND_MAX);
         newPoints->InsertNextPoint(p);
      }
      inputDataSet->SetPoints(newPoints);
      newPoints->Delete();
      renWin->Render();
   }
}

void main ()
{
   // Create the vtk renderer stuff
   vtkRenderer *ren = vtkRenderer::New();
   renWin = vtkRenderWindow::New();
      renWin->AddRenderer(ren);
      renWin->SetSize (300,300);
   vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
      iren->SetRenderWindow(renWin);
      iren->SetUserMethod(UserMethod, 0);

   // Create points for the starting positions of the particles
   vtkFloatPoints *startPoints = vtkFloatPoints::New();
   float sp[8][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, 
                     {1, 1, 0}, {1, 0, 1}, {0, 1, 1,}, {1, 1, 1}};
   for (int i=0; i<8; i++)
      startPoints->InsertNextPoint(sp[i]);

   // Create a data set. Load the starting points
   inputDataSet = vtkPolyData::New();
   inputDataSet->SetPoints(startPoints);
   startPoints->Delete();

   // Create a vtkSphereSource object to represent a particle
   vtkSphereSource *sphereSource = vtkSphereSource::New();
      sphereSource->SetThetaResolution(8);
      sphereSource->SetPhiResolution(8);
      sphereSource->SetRadius(.05);

   // Create the "cloud" of particles, using a vtkGlyph3D object
   vtkGlyph3D *cloud = vtkGlyph3D::New();
      cloud->SetInput(inputDataSet);
      cloud->SetSource(sphereSource->GetOutput());

   // Create the mapper and actor and finish up the visualization pipeline
   vtkPolyDataMapper *cloudMapper = vtkPolyDataMapper::New();
      cloudMapper->SetInput(cloud->GetOutput());
   vtkActor *cloudActor = vtkActor::New();
      cloudActor->SetMapper(cloudMapper);
      cloudActor->GetProperty()->SetColor(0, 1, 1);
   ren->AddActor(cloudActor);
   ren->SetBackground(1,1,1);

   // interact with data
   renWin->Render();
   iren->Start();

   // Clean up
   ren->Delete();
   renWin->Delete();
   inputDataSet->Delete();
   sphereSource->Delete();
   cloud->Delete();
   cloudMapper->Delete();
   cloudActor->Delete();
}
