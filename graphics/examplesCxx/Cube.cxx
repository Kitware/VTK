#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkScalars.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  int i;
  static float x[8][3]={{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0},
                        {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}};
  static vtkIdType pts[6][4]={{0,1,2,3}, {4,5,6,7}, {0,1,5,4},
                        {1,2,6,5}, {2,3,7,6}, {3,0,4,7}};
  
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkPolyData *cube = vtkPolyData::New();
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *polys = vtkCellArray::New();
  vtkScalars *scalars = vtkScalars::New(VTK_INT);

  for (i=0; i<8; i++) points->InsertPoint(i,x[i]);
  for (i=0; i<6; i++) polys->InsertNextCell(4,pts[i]);
  for (i=0; i<8; i++) scalars->InsertScalar(i,i);

  cube->SetPoints(points);
  points->Delete();
  cube->SetPolys(polys);
  polys->Delete();
  cube->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
      cubeMapper->SetInput(cube);
      cubeMapper->SetScalarRange(0,7);
  vtkActor *cubeActor = vtkActor::New();
      cubeActor->SetMapper(cubeMapper);

  vtkCamera *camera = vtkCamera::New();
      camera->SetPosition(1,1,1);
      camera->SetFocalPoint(0,0,0);
  renderer->AddActor(cubeActor);
      renderer->SetActiveCamera(camera);
      renderer->ResetCamera();
      renderer->SetBackground(1,1,1);
  
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  cube->Delete();
  points->Delete();
  polys->Delete();
  scalars->Delete();
  cubeMapper->Delete();
  cubeActor->Delete();
  camera->Delete();
}
