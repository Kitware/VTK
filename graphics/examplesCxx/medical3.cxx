#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "vtkStripper.h"
#include "vtkLookupTable.h"
#include "vtkExtractVOI.h"
#include "vtkPlaneSource.h"
#include "vtkContourFilter.h"

#include "SaveImage.h"

void main( int argc, char *argv[] )
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

  // create a b/w lookup table
  vtkLookupTable *bwLut = vtkLookupTable::New();
    bwLut->SetTableRange (0, 2000);
    bwLut->SetSaturationRange (0, 0);
    bwLut->SetHueRange (0, 0);
    bwLut->SetValueRange (0, 1);
    bwLut->Build ();

  // create a hue lookup table
  vtkLookupTable *hueLut = vtkLookupTable::New();
    hueLut->SetTableRange (0, 2000);
    hueLut->SetHueRange (0, 1);
    hueLut->SetSaturationRange (1, 1);
    hueLut->SetValueRange (1, 1);
        hueLut->Build();

  // create a saturation lookup table
  vtkLookupTable *satLut = vtkLookupTable::New();
    satLut->SetTableRange (0, 2000);
    satLut->SetHueRange (.6, .6);
    satLut->SetSaturationRange (0, 1);
    satLut->SetValueRange (1, 1);
        satLut->Build();

  // sagittal
  vtkExtractVOI *saggitalSection = vtkExtractVOI::New();
    saggitalSection->SetVOI (32,32, 0,63, 0, 93);
    saggitalSection->SetInput (v16->GetOutput());
  vtkTexture *saggitalTexture = vtkTexture::New();
    saggitalTexture->SetInput(saggitalSection->GetOutput());
    saggitalTexture->InterpolateOn();
    saggitalTexture->SetLookupTable (bwLut);
    saggitalTexture->MapColorScalarsThroughLookupTableOn();
  vtkPlaneSource *saggitalPlane = vtkPlaneSource::New();
    saggitalPlane->SetXResolution(1);
    saggitalPlane->SetYResolution(1);
    saggitalPlane->SetOrigin(3.2*32.0, 0.0, 0.0);
    saggitalPlane->SetPoint1(3.2*32.0, 3.2*63.0, 0.0);
    saggitalPlane->SetPoint2(3.2*32.0, 0.0, 1.5*92.0);
  vtkPolyDataMapper *saggitalMapper = vtkPolyDataMapper::New();
    saggitalMapper->SetInput(saggitalPlane->GetOutput());
    saggitalMapper->ImmediateModeRenderingOn();
  vtkActor *sagittal = vtkActor::New();
    sagittal->SetMapper(saggitalMapper);
    sagittal->SetTexture(saggitalTexture);

  // axial
  vtkExtractVOI *axialSection = vtkExtractVOI::New();
    axialSection->SetVOI (0,63, 0,63, 46,46);
    axialSection->SetInput (v16->GetOutput());
  vtkTexture *axialTexture = vtkTexture::New();
    axialTexture->SetInput(axialSection->GetOutput());
    axialTexture->InterpolateOn();
    axialTexture->SetLookupTable (hueLut);
    axialTexture->MapColorScalarsThroughLookupTableOn();
  vtkPlaneSource *axialPlane = vtkPlaneSource::New();
    axialPlane->SetXResolution(1);
    axialPlane->SetYResolution(1);
    axialPlane->SetOrigin(0.0, 0.0, 1.5*46);
    axialPlane->SetPoint1(3.2*63, 0.0, 1.5*46);
    axialPlane->SetPoint2(0.0, 3.2*63, 1.5*46.0);
  vtkPolyDataMapper *axialMapper = vtkPolyDataMapper::New();
    axialMapper->SetInput(axialPlane->GetOutput());
    axialMapper->ImmediateModeRenderingOn();
  vtkActor *axial = vtkActor::New();
    axial->SetMapper(axialMapper);
    axial->SetTexture(axialTexture);

  // coronal
  vtkExtractVOI *coronalSection = vtkExtractVOI::New();
    coronalSection->SetVOI (0,63, 32,32, 0,92);
    coronalSection->SetInput (v16->GetOutput());
  vtkTexture *coronalTexture = vtkTexture::New();
    coronalTexture->SetInput(coronalSection->GetOutput());
    coronalTexture->InterpolateOn();
    coronalTexture->SetLookupTable (satLut);
    coronalTexture->MapColorScalarsThroughLookupTableOn();
  vtkPlaneSource *coronalPlane = vtkPlaneSource::New();
    coronalPlane->SetXResolution(1);
    coronalPlane->SetYResolution(1);
    coronalPlane->SetOrigin(0.0, 3.2*32, 0.0);
    coronalPlane->SetPoint1(3.2*63, 3.2*32, 0.0);
    coronalPlane->SetPoint2(0.0, 3.2*32, 1.5*92.0);
  vtkPolyDataMapper *coronalMapper = vtkPolyDataMapper::New();
    coronalMapper->SetInput(coronalPlane->GetOutput());
    coronalMapper->ImmediateModeRenderingOn();
  vtkActor *coronal = vtkActor::New();
    coronal->SetMapper(coronalMapper);
    coronal->SetTexture(coronalTexture);

  // create a camera with the correct view up
  vtkCamera *aCamera = vtkCamera::New();
    aCamera->SetViewUp (0, 0, -1);
    aCamera->SetPosition (0, 1, 0);
    aCamera->SetFocalPoint (0, 0, 0);

  // now, tell the renderer our actors
  aRenderer->AddActor(outline);
  aRenderer->AddActor(sagittal);
  aRenderer->AddActor(axial);
  aRenderer->AddActor(coronal);
  aRenderer->AddActor(skin);
  aRenderer->AddActor(bone);

  // turn off bone for this examples
  bone->VisibilityOff();

  // set skin to semi-transparent
  skin->GetProperty()->SetOpacity(0.5);

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
