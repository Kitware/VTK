#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPLOT3DReader.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkLineSource.h"
#include "vtkDashedStreamLine.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  float range[2], c[3];
  float maxVelocity, maxTime = 0;
  
  vtkRenderer *aren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkPLOT3DReader *reader = vtkPLOT3DReader::New();
    //reader->DebugOn();
    reader->SetXYZFileName("../../../vtkdata/bluntfinxyz.bin");
    reader->SetQFileName("../../../vtkdata/bluntfinq.bin");
    reader->SetFileFormat(VTK_WHOLE_SINGLE_GRID_NO_IBLANKING);
    reader->Update(); //force a read to occur
    reader->GetOutput()->GetCenter(c);
    reader->GetOutput()->GetLength();
  if ( reader->GetOutput()->GetPointData()->GetScalars() )
    {
    reader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
    }
  if ( reader->GetOutput()->GetPointData()->GetVectors() )
    {
    maxVelocity = reader->GetOutput()->GetPointData()->GetVectors()->GetMaxNorm();
    maxTime = 6.0*reader->GetOutput()->GetLength()/maxVelocity ;
    }

  vtkStructuredGridOutlineFilter *outlineF = vtkStructuredGridOutlineFilter::New();
    outlineF->SetInput(reader->GetOutput());
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outlineF->GetOutput());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(outlineMapper);
    outline->GetProperty()->SetColor(1.0,1.0,1.0);
//
// Some geometry for context
//
  vtkStructuredGridGeometryFilter *wall = vtkStructuredGridGeometryFilter::New();
    wall->SetInput(reader->GetOutput());
    wall->SetExtent(0, 100, 0, 100, 0, 0);
  vtkPolyDataMapper *wallMap = vtkPolyDataMapper::New();
    wallMap->SetInput(wall->GetOutput());
    wallMap->ScalarVisibilityOff();
  vtkActor *wallActor = vtkActor::New();
    wallActor->SetMapper(wallMap);
    wallActor->GetProperty()->SetColor(0.2,0.2,0.2);

  vtkStructuredGridGeometryFilter *fin = vtkStructuredGridGeometryFilter::New();
    fin->SetInput(reader->GetOutput());
    fin->SetExtent(0,100,0,0,0,100);
  vtkPolyDataMapper *finMap = vtkPolyDataMapper::New();
    finMap->SetInput(fin->GetOutput());
    finMap->ScalarVisibilityOff();
  vtkActor *finActor = vtkActor::New();
    finActor->SetMapper(finMap);
    finActor->GetProperty()->SetColor(0.4, 0.4, 0.4);
//
// regular streamlines
//
  vtkLineSource *line1 = vtkLineSource::New();
    line1->SetResolution(25);
    line1->SetPoint1(-6.36, 0.25, 0.06);
    line1->SetPoint2(-6.36, 0.25, 5.37);
  vtkPolyDataMapper *rakeMapper = vtkPolyDataMapper::New();
    rakeMapper->SetInput(line1->GetOutput());
  vtkActor *rake1 = vtkActor::New();
    rake1->SetMapper(rakeMapper);
    rake1->GetProperty()->SetColor(1.0,1.0,1.0);

  vtkDashedStreamLine *streamers = vtkDashedStreamLine::New();
  //  streamers->DebugOn();
    streamers->SetInput(reader->GetOutput());
    streamers->SetSource(line1->GetOutput());
    streamers->SetMaximumPropagationTime(maxTime);
    streamers->SetStepLength(maxTime/150.0);
    streamers->SetDashFactor(0.50);
    streamers->SetIntegrationStepLength(0.2);
    streamers->Update();
    //streamers->DebugOff();

  vtkPolyDataMapper *streamersMapper = vtkPolyDataMapper::New();
    streamersMapper->SetInput(streamers->GetOutput());
    streamersMapper->SetScalarRange(range);

  vtkActor *lines = vtkActor::New();
    lines->SetMapper(streamersMapper);

  aren->AddActor(outline);
  aren->AddActor(wallActor);
  aren->AddActor(finActor);
  aren->AddActor(rake1);
  aren->AddActor(lines);
  aren->SetBackground(0.0,0.0,0.0);
  aren->GetActiveCamera()->Elevation(30.0);
  aren->GetActiveCamera()->Azimuth(30.0);
  aren->GetActiveCamera()->Zoom(2.0);
  aren->GetActiveCamera()->SetClippingRange(1,1000);

  renWin->SetSize(300,150);
  renWin->Render();

  SAVEIMAGE( renWin );

  // interact with data
  iren->Start();

  // Clean up
  aren->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  outlineF->Delete();
  outlineMapper->Delete();
  outline->Delete();
  wall->Delete();
  wallMap->Delete();
  wallActor->Delete();
  fin->Delete();
  finMap->Delete();
  finActor->Delete();
  line1->Delete();
  rakeMapper->Delete();
  rake1->Delete();
  streamers->Delete();
  streamersMapper->Delete();
  lines->Delete();

  return( 0 );
}
