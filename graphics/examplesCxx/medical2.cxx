#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "vtkStripper.h"
#include "vtkContourFilter.h"

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
  vtkContourFilter *skinExtractor = vtkContourFilter::New();
    skinExtractor->SetInput(v16->GetOutput());
    skinExtractor->SetValue(0, 500);
  vtkStripper *skinStripper = vtkStripper::New();
    skinStripper->SetInput(skinExtractor->GetOutput());
  vtkPolyDataMapper *skinMapper = vtkPolyDataMapper::New();
    skinMapper->SetInput(skinStripper->GetOutput());
    skinMapper->ScalarVisibilityOff();
  vtkActor *skin = vtkActor::New();
    skin->SetMapper(skinMapper);
    skin->GetProperty()->SetDiffuseColor(1, .49, .25);
    skin->GetProperty()->SetSpecular(.3);
    skin->GetProperty()->SetSpecularPower(20);

  // extract the bone
  vtkContourFilter *boneExtractor = vtkContourFilter::New();
    boneExtractor->SetInput(v16->GetOutput());
    boneExtractor->SetValue(0, 1150);
  vtkStripper *boneStripper = vtkStripper::New();
    boneStripper->SetInput(boneExtractor->GetOutput());
  vtkPolyDataMapper *boneMapper = vtkPolyDataMapper::New();
    boneMapper->SetInput(boneStripper->GetOutput());
    boneMapper->ScalarVisibilityOff();
  vtkActor *bone = vtkActor::New();
    bone->SetMapper(boneMapper);
    bone->GetProperty()->SetDiffuseColor(1, 1, .9412);

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
  aRenderer->AddActor(bone);

  aRenderer->SetActiveCamera(aCamera);
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);
  aRenderer->SetBackground(1,1,1);
  aRenderer->ResetCameraClippingRange();
  
  
  // interact with data
  renWin->SetSize(300, 300);
  renWin->Render();

  SAVEIMAGE( renWin );
  iren->Start(); 
}
