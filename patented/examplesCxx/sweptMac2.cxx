#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkImplicitModeller.h"
#include "vtkTransformCollection.h"
#include "vtkTransform.h"
#include "vtkSweptSurface.h"
#include "vtkMarchingContourFilter.h"
#include "vtkPolyDataMapper.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // create mace
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);

  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(6);

  vtkGlyph3D *glyph = vtkGlyph3D::New();
    glyph->SetInput(sphere->GetOutput());
    glyph->SetSource(cone->GetOutput());
    glyph->SetVectorModeToUseNormal();
    glyph->SetScaleModeToScaleByVector();
    glyph->SetScaleFactor(0.25);

  vtkAppendPolyData *append = vtkAppendPolyData::New();
    append->AddInput(sphere->GetOutput());
    append->AddInput(glyph->GetOutput());

  vtkPolyDataMapper *maceMapper = vtkPolyDataMapper::New();
    maceMapper->SetInput(append->GetOutput());

  vtkActor *maceActor = vtkActor::New();
    maceActor->SetMapper(maceMapper);
    maceActor->GetProperty()->SetColor(1,0,0);

  // create implicit model of mace
  vtkImplicitModeller *imp = vtkImplicitModeller::New();
    imp->SetInput(append->GetOutput());
    imp->SetSampleDimensions(50,50,50);
    imp->SetMaximumDistance(0.125);
    imp->AdjustBoundsOn();
    imp->SetAdjustDistance(0.125);
    imp->SetProcessModeToPerVoxel();

  // create swept surface
  vtkTransformCollection *transforms = vtkTransformCollection::New();

  vtkTransform *t1 = vtkTransform::New();
    t1->Identity();

  vtkTransform *t2 = vtkTransform::New();
    t2->Translate(0,0,2.5);
    t2->RotateZ(90.0);

  transforms->AddItem(t1);
  transforms->AddItem(t2);

  vtkSweptSurface *sweptSurfaceFilter = vtkSweptSurface::New();
    sweptSurfaceFilter->SetInput(imp->GetOutput());
    sweptSurfaceFilter->SetTransforms(transforms);
    sweptSurfaceFilter->SetSampleDimensions(60,60,100);
    sweptSurfaceFilter->SetNumberOfInterpolationSteps(30);
    sweptSurfaceFilter->AdjustBoundsOn();
    sweptSurfaceFilter->SetAdjustDistance(0.5);

  vtkMarchingContourFilter *iso = vtkMarchingContourFilter::New();
    iso->SetInput(sweptSurfaceFilter->GetOutput());
    iso->SetValue(0, 0.075);

  vtkPolyDataMapper *sweptSurfaceMapper = vtkPolyDataMapper::New();
    sweptSurfaceMapper->SetInput(iso->GetOutput());
    sweptSurfaceMapper->ScalarVisibilityOff();

  vtkActor *sweptSurface = vtkActor::New();
    sweptSurface->SetMapper(sweptSurfaceMapper);
    sweptSurface->GetProperty()->SetColor(0.8667,0.6275,0.8667);

  ren->AddActor(maceActor);
  ren->AddActor(sweptSurface);
  ren->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // allow keyboard manipulation of object
  renWin->Render();

  SAVEIMAGE( renWin );

  iren->Start();
}


