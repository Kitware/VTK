#include "vtk.h"

main ()
{
  float range[2];
  char c;

  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren1);
    renWin->DoubleBufferOff();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  //
  // Read data
  //
  vtkStructuredPointsReader *reader;
    reader->SetFileName("../../../vtkdata/ironProt.vtk");
    reader->Update();
    reader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  //
  // Create outline
  //
  vtkOutlineFilter *outline = vtkOutlineFilter::New();
    outline->SetInput(reader->GetOutput());
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());
  vtkActor *outline1Actor = vtkActor::New();
    outline1Actor->SetMapper(outlineMapper);

  ren1->SetBackground(0.1,0.2,0.4);
  ren1->AddActor(outline1Actor);
  renWin->SetSize(150,150);
  renWin->Render();
  ren1->GetActiveCamera()->Zoom(1.5);

  vtkVolume *vol = vtkVolume::New();
    vol->SetVolumeMapper(reader->GetOutput());
    vol->GetLookupTable()->SetAlphaRange(0,0.3);
    vol->SetScalarRange(range);

  vtkVolumeRenderer *volRen = vtkVolumeRenderer::New();
    volRen->AddVolume(vol);
    volRen->SetStepSize(0.3);

  ren1->SetVolumeRenderer(volRen);

  // interact with data
  renWin->Render();

  cin >> c;

  // Clean up
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outline1Actor->Delete();
  vol->Delete();
  volRen->Delete();
}
