#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredGridReader.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkLineSource.h"
#include "vtkStreamLine.h"
#include "vtkCamera.h"
#include "vtkRungeKutta4.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  float range[2];
  float maxVelocity, maxTime = 0;
  
  vtkRenderer *aren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aren);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
//
// Read data
//
  vtkStructuredGridReader *reader = vtkStructuredGridReader::New();
    //reader->DebugOn();
    reader->SetFileName("../../../vtkdata/kitchen.vtk");
    reader->Update(); //force a read to occur
    reader->GetOutput()->GetLength();

  if ( reader->GetOutput()->GetPointData()->GetScalars() )
    {
    reader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
    }
  if ( reader->GetOutput()->GetPointData()->GetVectors() )
    {
    maxVelocity = reader->GetOutput()->GetPointData()->GetVectors()->GetMaxNorm();
    maxTime = 35.0*reader->GetOutput()->GetLength()/maxVelocity ;
    }
//
// Outline around data
//
  vtkStructuredGridOutlineFilter *outlineF = vtkStructuredGridOutlineFilter::New();
    outlineF->SetInput(reader->GetOutput());
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outlineF->GetOutput());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(outlineMapper);
    outline->GetProperty()->SetColor(1,1,1);
//
// Set up shaded surfaces (i.e., supporting geometry)
//
  vtkStructuredGridGeometryFilter *doorGeom = vtkStructuredGridGeometryFilter::New();
    doorGeom->SetInput(reader->GetOutput());
    doorGeom->SetExtent(27,27,14,18,0,11);
  vtkPolyDataMapper *mapDoor = vtkPolyDataMapper::New();
    mapDoor->SetInput(doorGeom->GetOutput());
    mapDoor->ScalarVisibilityOff();
  vtkActor *door = vtkActor::New();
    door->SetMapper(mapDoor);
    door->GetProperty()->SetColor(.59,.427,.3);

  vtkStructuredGridGeometryFilter *window1Geom = vtkStructuredGridGeometryFilter::New();
    window1Geom->SetInput(reader->GetOutput());
    window1Geom->SetExtent(0,0,9,18,6,12);
  vtkPolyDataMapper *mapWindow1 = vtkPolyDataMapper::New();
    mapWindow1->SetInput(window1Geom->GetOutput());
    mapWindow1->ScalarVisibilityOff();
  vtkActor *window1 = vtkActor::New();
    window1->SetMapper(mapWindow1);
    window1->GetProperty()->SetColor(.3,.3,.5);

  vtkStructuredGridGeometryFilter *window2Geom = vtkStructuredGridGeometryFilter::New(); 
    window2Geom->SetInput(reader->GetOutput());
    window2Geom->SetExtent(5,12,23,23,6,12);
  vtkPolyDataMapper *mapWindow2 = vtkPolyDataMapper::New();
    mapWindow2->SetInput(window2Geom->GetOutput());
    mapWindow2->ScalarVisibilityOff();
  vtkActor *window2 = vtkActor::New();
    window2->SetMapper(mapWindow2);
    window2->GetProperty()->SetColor(.3,.3,.5);

  vtkStructuredGridGeometryFilter *klower1Geom = vtkStructuredGridGeometryFilter::New();
    klower1Geom->SetInput(reader->GetOutput());
    klower1Geom->SetExtent(17,17,0,11,0,6);
  vtkPolyDataMapper *mapKlower1 = vtkPolyDataMapper::New();
    mapKlower1->SetInput(klower1Geom->GetOutput());
    mapKlower1->ScalarVisibilityOff();
  vtkActor *klower1 = vtkActor::New();
    klower1->SetMapper(mapKlower1);
    klower1->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *klower2Geom = vtkStructuredGridGeometryFilter::New();
    klower2Geom->SetInput(reader->GetOutput());
    klower2Geom->SetExtent(19,19,0,11,0,6);
  vtkPolyDataMapper *mapKlower2 = vtkPolyDataMapper::New();
    mapKlower2->SetInput(klower2Geom->GetOutput());
    mapKlower2->ScalarVisibilityOff();
  vtkActor *klower2 = vtkActor::New();
    klower2->SetMapper(mapKlower2);
    klower2->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *klower3Geom = vtkStructuredGridGeometryFilter::New();
    klower3Geom->SetInput(reader->GetOutput());
    klower3Geom->SetExtent(17,19,0,0,0,6);
  vtkPolyDataMapper *mapKlower3 = vtkPolyDataMapper::New();
    mapKlower3->SetInput(klower3Geom->GetOutput());
    mapKlower3->ScalarVisibilityOff();
  vtkActor *klower3 = vtkActor::New();
    klower3->SetMapper(mapKlower3);
    klower3->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *klower4Geom = vtkStructuredGridGeometryFilter::New();
    klower4Geom->SetInput(reader->GetOutput());
    klower4Geom->SetExtent(17,19,11,11,0,6);
  vtkPolyDataMapper *mapKlower4 = vtkPolyDataMapper::New();
    mapKlower4->SetInput(klower4Geom->GetOutput());
    mapKlower4->ScalarVisibilityOff();
  vtkActor *klower4 = vtkActor::New();
    klower4->SetMapper(mapKlower4);
    klower4->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *klower5Geom = vtkStructuredGridGeometryFilter::New();
    klower5Geom->SetInput(reader->GetOutput());
    klower5Geom->SetExtent(17,19,0,11,0,0);
  vtkPolyDataMapper *mapKlower5 = vtkPolyDataMapper::New();
    mapKlower5->SetInput(klower5Geom->GetOutput());
    mapKlower5->ScalarVisibilityOff();
  vtkActor *klower5 = vtkActor::New();
    klower5->SetMapper(mapKlower5);
    klower5->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *klower6Geom = vtkStructuredGridGeometryFilter::New();
    klower6Geom->SetInput(reader->GetOutput());
    klower6Geom->SetExtent(17,19,0,7,6,6);
  vtkPolyDataMapper *mapKlower6 = vtkPolyDataMapper::New();
    mapKlower6->SetInput(klower6Geom->GetOutput());
    mapKlower6->ScalarVisibilityOff();
  vtkActor *klower6 = vtkActor::New();
    klower6->SetMapper(mapKlower6);
    klower6->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *klower7Geom = vtkStructuredGridGeometryFilter::New();
    klower7Geom->SetInput(reader->GetOutput());
    klower7Geom->SetExtent(17,19,9,11,6,6);
  vtkPolyDataMapper *mapKlower7 = vtkPolyDataMapper::New();
    mapKlower7->SetInput(klower7Geom->GetOutput());
    mapKlower7->ScalarVisibilityOff();
  vtkActor *klower7 = vtkActor::New();
    klower7->SetMapper(mapKlower7);
    klower7->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *hood1Geom = vtkStructuredGridGeometryFilter::New();
    hood1Geom->SetInput(reader->GetOutput());
    hood1Geom->SetExtent(17,17,0,11,11,16);
  vtkPolyDataMapper *mapHood1 = vtkPolyDataMapper::New();
    mapHood1->SetInput(hood1Geom->GetOutput());
    mapHood1->ScalarVisibilityOff();
  vtkActor *hood1 = vtkActor::New();
    hood1->SetMapper(mapHood1);
    hood1->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *hood2Geom = vtkStructuredGridGeometryFilter::New();
    hood2Geom->SetInput(reader->GetOutput());
    hood2Geom->SetExtent(19,19,0,11,11,16);
  vtkPolyDataMapper *mapHood2 = vtkPolyDataMapper::New();
    mapHood2->SetInput(hood2Geom->GetOutput());
    mapHood2->ScalarVisibilityOff();
  vtkActor *hood2 = vtkActor::New();
    hood2->SetMapper(mapHood2);
    hood2->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *hood3Geom = vtkStructuredGridGeometryFilter::New();
    hood3Geom->SetInput(reader->GetOutput());
    hood3Geom->SetExtent(17,19,0,0,11,16);
  vtkPolyDataMapper *mapHood3 = vtkPolyDataMapper::New();
    mapHood3->SetInput(hood3Geom->GetOutput());
    mapHood3->ScalarVisibilityOff();
  vtkActor *hood3 = vtkActor::New();
    hood3->SetMapper(mapHood3);
    hood3->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *hood4Geom = vtkStructuredGridGeometryFilter::New();
    hood4Geom->SetInput(reader->GetOutput());
    hood4Geom->SetExtent(17,19,11,11,11,16);
  vtkPolyDataMapper *mapHood4 = vtkPolyDataMapper::New();
    mapHood4->SetInput(hood4Geom->GetOutput());
    mapHood4->ScalarVisibilityOff();
  vtkActor *hood4 = vtkActor::New();
    hood4->SetMapper(mapHood4);
    hood4->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *hood6Geom = vtkStructuredGridGeometryFilter::New();
    hood6Geom->SetInput(reader->GetOutput());
    hood6Geom->SetExtent(17,19,0,11,16,16);
  vtkPolyDataMapper *mapHood6 = vtkPolyDataMapper::New();
    mapHood6->SetInput(hood6Geom->GetOutput());
    mapHood6->ScalarVisibilityOff();
  vtkActor *hood6 = vtkActor::New();
    hood6->SetMapper(mapHood6);
    hood6->GetProperty()->SetColor(.8,.8,.6);

  vtkStructuredGridGeometryFilter *cookingPlateGeom = vtkStructuredGridGeometryFilter::New();
    cookingPlateGeom->SetInput(reader->GetOutput());
    cookingPlateGeom->SetExtent(17,19,7,9,6,6);
  vtkPolyDataMapper *mapCookingPlate = vtkPolyDataMapper::New();
    mapCookingPlate->SetInput(cookingPlateGeom->GetOutput());
    mapCookingPlate->ScalarVisibilityOff();
  vtkActor *cookingPlate = vtkActor::New();
    cookingPlate->SetMapper(mapCookingPlate);
    cookingPlate->GetProperty()->SetColor(.9,.1,.1);

  vtkStructuredGridGeometryFilter *filterGeom = vtkStructuredGridGeometryFilter::New();
    filterGeom->SetInput(reader->GetOutput());
    filterGeom->SetExtent(17,19,7,9,11,11);
  vtkPolyDataMapper *mapFilter = vtkPolyDataMapper::New();
    mapFilter->SetInput(filterGeom->GetOutput());
    mapFilter->ScalarVisibilityOff();
  vtkActor *filter = vtkActor::New();
    filter->SetMapper(mapFilter);
    filter->GetProperty()->SetColor(.8,.6,.6);
//
// regular streamlines
//
  vtkLineSource *line = vtkLineSource::New();
    line->SetResolution(39);
    line->SetPoint1(0.08, 2.50, 0.71);
    line->SetPoint2(0.08, 4.50, 0.71);
  vtkPolyDataMapper *rakeMapper = vtkPolyDataMapper::New();
    rakeMapper->SetInput(line->GetOutput());
  vtkActor *rake = vtkActor::New();
    rake->SetMapper(rakeMapper);

  vtkRungeKutta4 *integ = vtkRungeKutta4::New();

  vtkStreamLine *streamers = vtkStreamLine::New();
    //streamers->DebugOn();
    streamers->SetInput(reader->GetOutput());
    streamers->SetSource(line->GetOutput());
    streamers->SetMaximumPropagationTime(maxTime);
    streamers->SetStepLength(maxTime/500.0);
    streamers->SetIntegrationStepLength(0.02);
    streamers->SetIntegrator(integ);
    streamers->Update();
    //streamers->DebugOff();
  vtkPolyDataMapper *streamersMapper = vtkPolyDataMapper::New();
    streamersMapper->SetInput(streamers->GetOutput());
    streamersMapper->SetScalarRange(range);
//    streamersMapper->ScalarVisibilityOff();
  vtkActor *lines = vtkActor::New();
    lines->SetMapper(streamersMapper);
    lines->GetProperty()->SetColor(0,0,0);

  aren->AddActor(outline);
  aren->AddActor(door);
  aren->AddActor(window1);
  aren->AddActor(window2);
  aren->AddActor(klower1);
  aren->AddActor(klower2);
  aren->AddActor(klower3);
  aren->AddActor(klower4);
  aren->AddActor(klower5);
  aren->AddActor(klower6);
  aren->AddActor(klower7);
  aren->AddActor(hood1);
  aren->AddActor(hood2);
  aren->AddActor(hood3);
  aren->AddActor(hood4);
  aren->AddActor(hood6);
  aren->AddActor(cookingPlate);
  aren->AddActor(filter);
  aren->AddActor(lines);
  aren->AddActor(rake);

  aren->SetBackground(0.1, 0.2, 0.4);

  vtkCamera *aCamera = vtkCamera::New();
  aren->SetActiveCamera(aCamera);
  aren->ResetCamera();

  aCamera->SetFocalPoint(3.505, 2.505, 1.255);
  aCamera->SetPosition(3.505, 24.6196, 1.255);
  aCamera->SetViewUp(0,0,1);

  renWin->SetSize(300,300);
  renWin->Render();

  SAVEIMAGE( renWin );

  // interact with data
  iren->Start();

  // Clean up
  integ->Delete();
  aren->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  outlineF->Delete();
  outlineMapper->Delete();
  outline->Delete();
  doorGeom->Delete();
  mapDoor->Delete();
  door->Delete();
  window1Geom->Delete();
  mapWindow1->Delete();
  window1->Delete();
  window2Geom->Delete();
  mapWindow2->Delete();
  window2->Delete();
  klower1Geom->Delete();
  mapKlower1->Delete();
  klower1->Delete();
  klower2Geom->Delete();
  mapKlower2->Delete();
  klower2->Delete();
  klower3Geom->Delete();
  mapKlower3->Delete();
  klower3->Delete();
  klower4Geom->Delete();
  mapKlower4->Delete();
  klower4->Delete();
  klower5Geom->Delete();
  mapKlower5->Delete();
  klower5->Delete();
  klower6Geom->Delete();
  mapKlower6->Delete();
  klower6->Delete();
  klower7Geom->Delete();
  mapKlower7->Delete();
  klower7->Delete();
  hood1Geom->Delete();
  mapHood1->Delete();
  hood1->Delete();
  hood2Geom->Delete();
  mapHood2->Delete();
  hood2->Delete();
  hood3Geom->Delete();
  mapHood3->Delete();
  hood3->Delete();
  hood4Geom->Delete();
  mapHood4->Delete();
  hood4->Delete();
  hood6Geom->Delete();
  mapHood6->Delete();
  hood6->Delete();
  cookingPlateGeom->Delete();
  mapCookingPlate->Delete();
  cookingPlate->Delete();
  filterGeom->Delete();
  mapFilter->Delete();
  filter->Delete();
  line->Delete();
  rakeMapper->Delete();
  rake->Delete();
  streamers->Delete();
  streamersMapper->Delete();
  lines->Delete();
  aCamera->Delete();
}
