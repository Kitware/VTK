#include "vtk.h"

main ()
{
  int i, j, k, kOffset, jOffset, offset;
  float x[3], v[3], rMin=0.5, rMax=1.0, deltaRad, deltaZ;
  float radius, theta;
  vtkMath math;
  static int dims[3]={13,11,11};
  
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkStructuredGrid *sgrid = vtkStructuredGrid::New();
      sgrid->SetDimensions(dims);

  vtkFloatVectors *vectors = vtkFloatVectors::New();
    vectors->Allocate(dims[0]*dims[1]*dims[2]);
  vtkFloatPoints *points = vtkFloatPoints::New();
    points->Allocate(dims[0]*dims[1]*dims[2]);
  
  deltaZ = 2.0 / (dims[2]-1);
  deltaRad = (rMax-rMin) / (dims[1]-1);
  v[2]=0.0;
  for ( k=0; k<dims[2]; k++)
    {
    x[2] = -1.0 + k*deltaZ;
    kOffset = k * dims[0] * dims[1];
    for (j=0; j<dims[1]; j++) 
      {
      radius = rMin + j*deltaRad;
      jOffset = j * dims[0];
      for (i=0; i<dims[0]; i++) 
        {
        theta = i * 15.0 * math.DegreesToRadians();
        x[0] = radius * cos(theta);
        x[1] = radius * sin(theta);
        v[0] = -x[1];
        v[1] = x[0];
        offset = i + jOffset + kOffset;
        points->InsertPoint(offset,x);
        vectors->InsertVector(offset,v);
        }
      }
    }
  sgrid->SetPoints(points);
  points->Delete();
  sgrid->GetPointData()->SetVectors(vectors);
  vectors->Delete();

  vtkHedgeHog *hedgehog = vtkHedgeHog::New();
      hedgehog->SetInput(sgrid);
      hedgehog->SetScaleFactor(0.1);

  vtkPolyDataMapper *sgridMapper = vtkPolyDataMapper::New();
      sgridMapper->SetInput(hedgehog->GetOutput());
  vtkActor *sgridActor = vtkActor::New();
      sgridActor->SetMapper(sgridMapper);
      sgridActor->GetProperty()->SetColor(0,0,0);

  renderer->AddActor(sgridActor);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(450,450);

  // interact with data
  renWin->Render();
  iren->Start();

  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sgrid->Delete();
  vectors->Delete();
  points->Delete();
  hedgehog->Delete();
  sgridMapper->Delete();
  sgridActor->Delete();
}
