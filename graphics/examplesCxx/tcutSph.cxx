//
// cut an outer sphere to reveal an inner sphere
//
#include "vtk.h"

main ()
{
  vtkRenderer *aren = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(aren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // hidden sphere
  vtkSphereSource *sphere1 = vtkSphereSource::New();
    sphere1->SetThetaResolution(12); sphere1->SetPhiResolution(12);
    sphere1->SetRadius(0.5);

  vtkPolyDataMapper *innerMapper = vtkPolyDataMapper::New();
    innerMapper->SetInput(sphere1->GetOutput());

  vtkActor *innerSphere = vtkActor::New();
    innerSphere->SetMapper(innerMapper);
    innerSphere->GetProperty()->SetColor (1, .9216, .8039);

  // sphere to texture
  vtkSphereSource *sphere2 = vtkSphereSource::New();
    sphere2->SetThetaResolution(24); sphere2->SetPhiResolution(24);
    sphere2->SetRadius(1.0);

  vtkPlanes *planes = vtkPlanes::New();
    float pts[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    vtkFloatPoints *points = vtkFloatPoints::New();
        points->InsertPoint (0, pts);
        points->InsertPoint (1, pts + 3);
    float nrms[6] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    vtkFloatNormals *normals = vtkFloatNormals::New();
        normals->InsertNormal (0, nrms);
        normals->InsertNormal (1, nrms + 3);
    planes->SetPoints (points);
    planes->SetNormals (normals);

  vtkImplicitTextureCoords *tcoords = vtkImplicitTextureCoords::New();
    tcoords->SetInput(sphere2->GetOutput());
    tcoords->SetRFunction(planes);

  vtkDataSetMapper *outerMapper = vtkDataSetMapper::New();
    outerMapper->SetInput(tcoords->GetOutput());

  vtkStructuredPointsReader *tmap = vtkStructuredPointsReader::New();
    tmap->SetFileName("../../../vtkdata/texThres.vtk");

  vtkTexture *texture = vtkTexture::New();
    texture->SetInput(tmap->GetOutput());
    texture->InterpolateOff();
    texture->RepeatOff ();

  vtkActor *outerSphere = vtkActor::New();
    outerSphere->SetMapper(outerMapper);
    outerSphere->SetTexture(texture);
    outerSphere->GetProperty()->SetColor (1, .6275, .4784);

  aren->AddActor(innerSphere);
  aren->AddActor(outerSphere);
  aren->SetBackground (0.4392,0.5020,0.5647);
  renWin->SetSize(500,500);

  // interact with data
  renWin->Render();

  iren->Start();

  // Clean up
  aren->Delete();
  renWin->Delete();
  iren->Delete();
  sphere1->Delete();
  innerMapper->Delete();
  innerSphere->Delete();
  sphere2->Delete();
  planes->Delete();
  points->Delete();
  normals->Delete();
  tcoords->Delete();
  outerMapper->Delete();
  tmap->Delete();
  texture->Delete();
  outerSphere->Delete();
}
