#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkPolyDataNormals.h"
#include "vtkContourFilter.h"

int main (int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return 1;
    }

  // create the renderer stuff
  vtkRenderer *aRenderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aRenderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // read the volume
  vtkVolume16Reader *v16 = vtkVolume16Reader::New();
    v16->SetDataDimensions (64,64);
    v16->SetImageRange (1,93);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix (argv[1]);
    v16->SetDataSpacing (3.2, 3.2, 1.5);

  // extract the skin
  vtkContourFilter *skinExtractor = vtkContourFilter::New();
    skinExtractor->SetInput((vtkDataSet *) v16->GetOutput());
    skinExtractor->SetValue(0, 500);
  vtkPolyDataNormals *skinNormals = vtkPolyDataNormals::New();
    skinNormals->SetInput(skinExtractor->GetOutput());
    skinNormals->SetFeatureAngle(60.0);
  vtkPolyDataMapper *skinMapper = vtkPolyDataMapper::New();
    skinMapper->SetInput(skinNormals->GetOutput());
    skinMapper->ScalarVisibilityOff();
  vtkActor *skin = vtkActor::New();
    skin->SetMapper(skinMapper);

  // get an outline
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInput((vtkDataSet *) v16->GetOutput());
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
    aCamera->ComputeViewPlaneNormal();

  // now, tell the renderer our actors
  aRenderer->AddActor(outline);
  aRenderer->AddActor(skin);
  aRenderer->SetActiveCamera(aCamera);
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);
  aRenderer->SetBackground(1,1,1);
  aRenderer->ResetCameraClippingRange ();

  // interact with data
  renWin->SetSize(640, 480);
  renWin->Render();
  iren->Start(); 
  return 0;
}
