#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  int i;
  static float x[27][3]={{0,0,0}, {1,0,0}, {2,0,0}, {0,1,0}, {1,1,0}, {2,1,0},
                         {0,0,1}, {1,0,1}, {2,0,1}, {0,1,1}, {1,1,1}, {2,1,1},
                         {0,1,2}, {1,1,2}, {2,1,2}, {0,1,3}, {1,1,3}, {2,1,3},
                         {0,1,4}, {1,1,4}, {2,1,4}, {0,1,5}, {1,1,5}, {2,1,5},
                         {0,1,6}, {1,1,6}, {2,1,6}};
  static vtkIdType pts[12][8]={{0, 1, 4, 3, 6, 7, 10, 9},
                         {1, 2, 5, 4, 7, 8, 11, 10},
                         {6, 10, 9, 12, 0, 0, 0, 0},
                         {8, 11, 10, 14, 0, 0, 0, 0},
                         {16, 17, 14, 13, 12, 15, 0, 0},
                         {18, 15, 19, 16, 20, 17, 0, 0},
                         {22, 23, 20, 19, 0, 0, 0, 0},
                         {21, 22, 18, 0, 0, 0, 0, 0},
                         {22, 19, 18, 0, 0, 0, 0, 0},
                         {23, 26, 0, 0, 0, 0, 0, 0},
                         {21, 24, 0, 0, 0, 0, 0, 0},
                         {25, 0, 0, 0, 0, 0, 0, 0}};

  
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkPoints *points = vtkPoints::New();
  for (i=0; i<27; i++) points->InsertPoint(i,x[i]);
  
  vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::New();
    ugrid->Allocate(100);
    ugrid->InsertNextCell(VTK_HEXAHEDRON, 8, pts[0]);
    ugrid->InsertNextCell(VTK_HEXAHEDRON, 8, pts[1]);
    ugrid->InsertNextCell(VTK_TETRA, 4, pts[2]);
    ugrid->InsertNextCell(VTK_TETRA, 4, pts[3]);
    ugrid->InsertNextCell(VTK_POLYGON, 6, pts[4]);
    ugrid->InsertNextCell(VTK_TRIANGLE_STRIP, 6, pts[5]);
    ugrid->InsertNextCell(VTK_QUAD, 4, pts[6]);
    ugrid->InsertNextCell(VTK_TRIANGLE, 3, pts[7]);
    ugrid->InsertNextCell(VTK_TRIANGLE, 3, pts[8]);
    ugrid->InsertNextCell(VTK_LINE, 2, pts[9]);
    ugrid->InsertNextCell(VTK_LINE, 2, pts[10]);
    ugrid->InsertNextCell(VTK_VERTEX, 1, pts[11]);

  ugrid->SetPoints(points);
  points->Delete();

  vtkDataSetMapper *ugridMapper = vtkDataSetMapper::New();
      ugridMapper->SetInput(ugrid);
      ugridMapper->ImmediateModeRenderingOn();
  vtkActor *ugridActor = vtkActor::New();
      ugridActor->SetMapper(ugridMapper);
      ugridActor->GetProperty()->SetColor(.8,.8,.8);
      ugridActor->AddPosition(0,0.001,0);
  vtkActor *wireActor = vtkActor::New();
      wireActor->SetMapper(ugridMapper);
      wireActor->GetProperty()->SetRepresentationToWireframe();
      wireActor->GetProperty()->SetColor(0,0,0);

  renderer->AddActor(ugridActor);
      renderer->AddActor(wireActor);
      renderer->SetBackground(1,1,1);
      renderer->GetActiveCamera()->Elevation(60.0);
      renderer->GetActiveCamera()->Azimuth(30.0);
      renderer->GetActiveCamera()->Zoom(0.75);
  
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  points->Delete();
  ugrid->Delete();
  ugridMapper->Delete();
  ugridActor->Delete();
  wireActor->Delete();
}
