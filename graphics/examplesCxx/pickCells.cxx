#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPointPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkShrinkFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkIdList.h"

static void PickCells(void *);
static vtkActor *sphereActor;
static vtkPolyData *plateOutput;
static vtkUnstructuredGrid *cells;
static vtkRenderWindow *renWin;
static vtkActor *cellsActor, *plateActor;

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *renderer = vtkRenderer::New();
  renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);

  vtkPointPicker *picker = vtkPointPicker::New();
    picker->SetTolerance(0.01);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    iren->SetPicker(picker);

  // read data file
  vtkPolyDataReader *plate = vtkPolyDataReader::New();
    plate->SetFileName("../../../vtkdata/plate.vtk");
    //plate->DebugOn();
    plateOutput = plate->GetOutput();
  vtkPolyDataMapper *plateMapper = vtkPolyDataMapper::New();
    plateMapper->SetInput(plate->GetOutput());
  plateActor = vtkActor::New();
    plateActor->SetMapper(plateMapper);
    plateActor->GetProperty()->SetColor(0.5000,0.5400,0.5300);

  // create marker for pick
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
    sphere->SetRadius(0.01);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());
  sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);
    sphereActor->GetProperty()->SetColor(1,0,0);
    sphereActor->PickableOff();

  // create actor and mapper to display picked cells
  cells = vtkUnstructuredGrid::New();
  vtkShrinkFilter *shrink = vtkShrinkFilter::New();
    shrink->SetInput(cells);
    shrink->SetShrinkFactor(0.75);
  vtkDataSetMapper *cellsMapper = vtkDataSetMapper::New();
    cellsMapper->SetInput(shrink->GetOutput());
  cellsActor = vtkActor::New();
    cellsActor->SetMapper(cellsMapper);
    cellsActor->PickableOff();
    cellsActor->VisibilityOff();
    cellsActor->GetProperty()->SetColor(0.5000,0.5400,0.5300);

  renderer->AddActor(cellsActor);
  renderer->AddActor(plateActor);
  renderer->AddActor(sphereActor);
  renderer->SetBackground(1,1,1);
  renderer->GetActiveCamera()->Elevation(30.0);
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(0.75);

  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->SetEndPickMethod(PickCells,(void *)iren);
  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  picker->Delete();
  iren->Delete();
  plate->Delete();
  plateMapper->Delete();
  plateActor->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  cells->Delete();
  shrink->Delete();
  cellsMapper->Delete();
  cellsActor->Delete();
}

static void PickCells(void *arg)
{
  vtkRenderWindowInteractor *iren = (vtkRenderWindowInteractor *)arg;
  vtkPointPicker *picker = (vtkPointPicker *)iren->GetPicker();
  int i, cellId;
  vtkIdList *cellIds = vtkIdList::New(); cellIds->Allocate(12);
  vtkIdList *ptIds = vtkIdList::New(); ptIds->Allocate(12);

  sphereActor->SetPosition(picker->GetPickPosition());

  if ( picker->GetPointId() >= 0 )
    {
    cout << "Point id: " << picker->GetPointId() << "\n";
    cellsActor->VisibilityOn();
    plateActor->VisibilityOff();

    cells->Initialize();
    cells->Allocate(100);
    cells->SetPoints(plateOutput->GetPoints());

    plateOutput->GetPointCells(picker->GetPointId(), cellIds);
    for (i=0; i < cellIds->GetNumberOfIds(); i++)
      {
      cellId = cellIds->GetId(i);
      plateOutput->GetCellPoints(cellId, ptIds);
      cells->InsertNextCell(plateOutput->GetCellType(cellId), ptIds);
      }
    }
  else
    {
    cellsActor->VisibilityOff();
    plateActor->VisibilityOn();
    }

  renWin->Render();
  cellIds->Delete();
  ptIds->Delete();
}

