#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkElevationFilter.h"
#include "vtkLookupTable.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(12); sphere->SetPhiResolution(12);

  vtkTransform *aTransform = vtkTransform::New();
    aTransform->Scale(1,1.5,2);

  vtkTransformFilter *transFilter = vtkTransformFilter::New();
    transFilter->SetInput(sphere->GetOutput());
    transFilter->SetTransform(aTransform);  

  vtkElevationFilter *colorIt = vtkElevationFilter::New();
    colorIt->SetInput(transFilter->GetOutput());
    colorIt->SetLowPoint(0,0,-1);
    colorIt->SetHighPoint(0,0,1);

  vtkLookupTable *lut = vtkLookupTable::New();
    lut->SetHueRange(0,0);
    lut->SetSaturationRange(0,0);
    lut->SetValueRange(.1,1);

  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
    mapper->SetLookupTable(lut);
    mapper->SetInput(colorIt->GetOutput());

  vtkActor *actor = vtkActor::New();
    actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(1,1,1);
  renderer->GetActiveCamera()->Elevation(60.0);
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(1.3);

  renWin->SetSize(300,300);

  renWin->Render();

  SAVEIMAGE( renWin );

  // interact with data
  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  aTransform->Delete();
  transFilter->Delete();
  colorIt->Delete();
  lut->Delete();
  mapper->Delete();
  actor->Delete();
}
