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
#include "vtkStructuredPointsGeometryFilter.h"
#include "../patented/vtkMarchingCubes.h"

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
  vtkMarchingCubes *skinExtractor = vtkMarchingCubes::New();
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
  vtkMarchingCubes *boneExtractor = vtkMarchingCubes::New();
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

  // create a hue lookup table
  vtkLookupTable *hueLut = vtkLookupTable::New();
    hueLut->SetTableRange (0, 2000);
    hueLut->SetHueRange (0, 1);
    hueLut->SetSaturationRange (1, 1);
    hueLut->SetValueRange (1, 1);

  // create a saturation lookup table
  vtkLookupTable *satLut = vtkLookupTable::New();
    satLut->SetTableRange (0, 2000);
    satLut->SetHueRange (.6, .6);
    satLut->SetSaturationRange (0, 1);
    satLut->SetValueRange (1, 1);

  // sagittal
  vtkStructuredPointsGeometryFilter *saggitalSection = 
      vtkStructuredPointsGeometryFilter::New();
    saggitalSection->SetExtent (32,32, 0,63, 0, 93);
    saggitalSection->SetInput (v16->GetOutput());
  vtkPolyDataMapper *saggitalMapper = vtkPolyDataMapper::New();
    saggitalMapper->SetInput(saggitalSection->GetOutput());
    saggitalMapper->ScalarVisibilityOn();
    saggitalMapper->SetScalarRange (0, 2000);
    saggitalMapper->SetLookupTable (bwLut);
  vtkActor *sagittal = vtkActor::New();
    sagittal->SetMapper(saggitalMapper);

  // axial
  vtkStructuredPointsGeometryFilter *axialSection = 
      vtkStructuredPointsGeometryFilter::New();
    axialSection->SetExtent (0,63, 0,63, 46, 46);
    axialSection->SetInput (v16->GetOutput());
  vtkPolyDataMapper *axialMapper = vtkPolyDataMapper::New();
    axialMapper->SetInput(axialSection->GetOutput());
    axialMapper->ScalarVisibilityOn();
    axialMapper->SetScalarRange (0, 2000);
    axialMapper->SetLookupTable (hueLut);
  vtkActor *axial = vtkActor::New();
    axial->SetMapper(axialMapper);

  // coronal
  vtkStructuredPointsGeometryFilter *coronalSection = 
      vtkStructuredPointsGeometryFilter::New();
    coronalSection->SetExtent (0,63, 32, 32, 0, 92);
    coronalSection->SetInput (v16->GetOutput());
  vtkPolyDataMapper *coronalMapper = vtkPolyDataMapper::New();
    coronalMapper->SetInput(coronalSection->GetOutput());
    coronalMapper->ScalarVisibilityOn();
    coronalMapper->SetScalarRange (0, 2000);
    coronalMapper->SetLookupTable (satLut);
  vtkActor *coronal = vtkActor::New();
    coronal->SetMapper(coronalMapper);

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
