#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "../patented/vtkMarchingCubes.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  // create the renderer stuff
  vtkRenderer *aRenderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aRenderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // read the volume
  vtkVolume16Reader *v16 = vtkVolume16Reader::New();
    v16->SetDataDimensions(64,64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix ("../../../vtkdata/headsq/quarter");
    v16->SetImageRange(1, 93);
    v16->SetDataSpacing (3.2, 3.2, 1.5);

  // extract the skin
  vtkMarchingCubes *skinExtractor = vtkMarchingCubes::New();
    skinExtractor->SetInput(v16->GetOutput());
    skinExtractor->SetValue(0, 500);
  vtkPolyDataMapper *skinMapper = vtkPolyDataMapper::New();
    skinMapper->SetInput(skinExtractor->GetOutput());
    skinMapper->ScalarVisibilityOff();
  vtkActor *skin = vtkActor::New();
    skin->SetMapper(skinMapper);

  // get an outline
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInput(v16->GetOutput());
  vtkPolyDataMapper *mapOutline = vtkPolyDataMapper::New();
    mapOutline->SetInput(outlineData->GetOutput());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(mapOutline);
    outline->GetProperty()->SetColor(0,0,0);

  // create a camera with the correct view up
  vtkCamera *aCamera = vtkCamera::New();
    aCamera->SetViewUp (0, 0, -1);
    aCamera->SetPosition (0, 1, 0);
    aCamera->SetFocalPoint (0, 0, 0);

  // now, tell the renderer our actors
  aRenderer->AddActor(outline);
  aRenderer->AddActor(skin);
  aRenderer->SetActiveCamera(aCamera);
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);
  aRenderer->SetBackground(1,1,1);
  aRenderer->ResetCameraClippingRange();
  
  // interact with data
  renWin->SetSize( 300, 300);
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start(); 
}
