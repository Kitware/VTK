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
#include "vtkImageDataGeometryFilter.h"
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
    v16->SetDataDimensions(64,64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix (argv[1]);
    v16->SetImageRange(1, 93);
    v16->SetDataSpacing (3.2, 3.2, 1.5);

  // extract the skin
  vtkContourFilter *skinExtractor = vtkContourFilter::New();
    skinExtractor->SetInput((vtkDataSet *) v16->GetOutput());
    skinExtractor->SetValue(0, 500);
  vtkPolyDataNormals *skinNormals = vtkPolyDataNormals::New();
    skinNormals->SetInput(skinExtractor->GetOutput());
    skinNormals->SetFeatureAngle(60.0);
  vtkStripper *skinStripper = vtkStripper::New();
    skinStripper->SetInput(skinNormals->GetOutput());
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
    boneExtractor->SetInput((vtkDataSet *) v16->GetOutput());
    boneExtractor->SetValue(0, 1150);
  vtkPolyDataNormals *boneNormals = vtkPolyDataNormals::New();
    boneNormals->SetInput(boneExtractor->GetOutput());
    boneNormals->SetFeatureAngle(60.0);
  vtkStripper *boneStripper = vtkStripper::New();
    boneStripper->SetInput(boneNormals->GetOutput());
  vtkPolyDataMapper *boneMapper = vtkPolyDataMapper::New();
    boneMapper->SetInput(boneStripper->GetOutput());
    boneMapper->ScalarVisibilityOff();
  vtkActor *bone = vtkActor::New();
    bone->SetMapper(boneMapper);
    bone->GetProperty()->SetDiffuseColor(1, 1, .9412);

  // get an outline
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInput((vtkDataSet *) v16->GetOutput());
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
  vtkImageDataGeometryFilter *saggitalSection = 
      vtkImageDataGeometryFilter::New();
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
  vtkImageDataGeometryFilter *axialSection = 
      vtkImageDataGeometryFilter::New();
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
  vtkImageDataGeometryFilter *coronalSection = 
      vtkImageDataGeometryFilter::New();
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
    aCamera->ComputeViewPlaneNormal();

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
  aRenderer->ResetCameraClippingRange ();

  // interact with data
  renWin->SetSize(640, 480);
  renWin->Render();
  iren->Start(); 
  return 0;
}
