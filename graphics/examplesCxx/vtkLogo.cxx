//
// use implicit modeller to create a logo
//

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataReader.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkImplicitModeller.h"
#include "vtkContourFilter.h"
#include "vtkProperty.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *aRenderer = vtkRenderer::New();
  vtkRenderWindow *ourRenderingWindow = vtkRenderWindow::New();
      ourRenderingWindow->AddRenderer(aRenderer);
  vtkRenderWindowInteractor *ourInteractor = vtkRenderWindowInteractor::New();
      ourInteractor->SetRenderWindow(ourRenderingWindow);
  ourRenderingWindow->SetSize( 300, 300 );

  // read the geometry file containing the letter v
  vtkPolyDataReader *letterVBYU = vtkPolyDataReader::New();
      letterVBYU->SetFileName ("../../../vtkdata/v.vtk");

  // read the geometry file containing the letter t
  vtkPolyDataReader *letterTBYU = vtkPolyDataReader::New();
      letterTBYU->SetFileName ("../../../vtkdata/t.vtk");

  // read the geometry file containing the letter k
  vtkPolyDataReader *letterKBYU = vtkPolyDataReader::New();
      letterKBYU->SetFileName ("../../../vtkdata/k.vtk");

  // create a transform and transform filter for each letter
  vtkTransform *VTransform = vtkTransform::New();
  vtkTransformPolyDataFilter *VTransformFilter = vtkTransformPolyDataFilter::New();
    VTransformFilter->SetInput (letterVBYU->GetOutput());
    VTransformFilter->SetTransform (VTransform);

  vtkTransform *TTransform = vtkTransform::New();
  vtkTransformPolyDataFilter *TTransformFilter = vtkTransformPolyDataFilter::New();
    TTransformFilter->SetInput (letterTBYU->GetOutput());
    TTransformFilter->SetTransform (TTransform);

  vtkTransform *KTransform = vtkTransform::New();
  vtkTransformPolyDataFilter *KTransformFilter = vtkTransformPolyDataFilter::New();
    KTransformFilter->SetInput (letterKBYU->GetOutput());
    KTransformFilter->SetTransform (KTransform);

  // now append them all
  vtkAppendPolyData *appendAll = vtkAppendPolyData::New();
    appendAll->AddInput (VTransformFilter->GetOutput());
    appendAll->AddInput (TTransformFilter->GetOutput());
    appendAll->AddInput (KTransformFilter->GetOutput());

  // create normals
  vtkPolyDataNormals *logoNormals = vtkPolyDataNormals::New();
      logoNormals->SetInput (appendAll->GetOutput());
      logoNormals->SetFeatureAngle (60);

  // map to rendering primitives
  vtkPolyDataMapper *logoMapper = vtkPolyDataMapper::New();
      logoMapper->SetInput (logoNormals->GetOutput());

  // now an actor
  vtkActor *logo = vtkActor::New();
    logo->SetMapper (logoMapper);

  // now create an implicit model of the same letter
  vtkImplicitModeller *blobbyLogoImp = vtkImplicitModeller::New();
      blobbyLogoImp->SetInput (appendAll->GetOutput());
      blobbyLogoImp->SetMaximumDistance (.075);
      blobbyLogoImp->SetSampleDimensions (64,64,64); 
      blobbyLogoImp->SetAdjustDistance (0.05);

  // extract an iso surface
  vtkContourFilter *blobbyLogoIso = vtkContourFilter::New();
      blobbyLogoIso->SetInput (blobbyLogoImp->GetOutput());
      blobbyLogoIso->SetValue (1, 1.5);

  // map to rendering primitives
  vtkPolyDataMapper *blobbyLogoMapper = vtkPolyDataMapper::New();
      blobbyLogoMapper->SetInput (blobbyLogoIso->GetOutput());
      blobbyLogoMapper->ScalarVisibilityOff ();

  vtkProperty *tomato = vtkProperty::New();
      tomato->SetDiffuseColor(1, .3882, .2784);
      tomato->SetSpecular(.3);
      tomato->SetSpecularPower(20);

  vtkProperty *banana = vtkProperty::New();
      banana->SetDiffuseColor(.89, .81, .34);
      banana->SetDiffuse (.7);
      banana->SetSpecular(.4);
      banana->SetSpecularPower(20);

  // now an actor
  vtkActor *blobbyLogo = vtkActor::New();
    blobbyLogo->SetMapper (blobbyLogoMapper);
    blobbyLogo->SetProperty (banana);

  // position the letters

  VTransform->Translate (-16.0,0.0,12.5);
  VTransform->RotateY (40);

  KTransform->Translate (14.0, 0.0, 0.0);
  KTransform->RotateY (-40);

  // move the polygonal letters to the front
  logo->SetProperty (tomato);
  logo->SetPosition(0,0,6);
  
  aRenderer->AddActor(logo);
  aRenderer->AddActor(blobbyLogo);

  aRenderer->SetBackground(1,1,1);

  ourRenderingWindow->Render();

  SAVEIMAGE( ourRenderingWindow );

  // interact with data
  ourInteractor->Start();
}

