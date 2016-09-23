/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayRenderMesh.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can do simple mesh rendering with ospray
// and that VTK's many standard rendering modes (points, lines, surface, with
// a variety of color controls (actor, point, cell, texture) etc work as
// they should.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit.
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h
// -GL       => users OpenGL instead of OSPRay to render
// -type N   => where N is one of 0,1,2, or 3 makes meshes consisting of
//              points, wireframes, triangles (=the default) or triangle strips
// -rep N    => where N is one of 0,1 or 2 draws the meshes as points, lines
//              or surfaces

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayPass.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkTextureMapToSphere.h"
#include "vtkTransformTextureCoords.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertexGlyphFilter.h"

#include <vector>
#include <string>

#include "vtkOSPRayTestInteractor.h"

class renderable
{
public:
  vtkSphereSource *s;
  vtkPolyDataMapper *m;
  vtkActor *a;
  ~renderable()
  {
    s->Delete();
    m->Delete();
    a->Delete();
  }
};

renderable *MakeSphereAt(double x, double y, double z, int res,
                         int type, int rep, const char *name)
{
  vtkOSPRayTestInteractor::AddName(name);
  renderable *ret = new renderable;
  ret->s = vtkSphereSource::New();
  ret->s->SetEndTheta(180); //half spheres better show variation and f and back
  ret->s->SetStartPhi(30);
  ret->s->SetEndPhi(150);
  ret->s->SetPhiResolution(res);
  ret->s->SetThetaResolution(res);
  ret->s->SetCenter(x,y,z);
  //make texture coordinate
  vtkSmartPointer<vtkTextureMapToSphere> tc =
    vtkSmartPointer<vtkTextureMapToSphere>::New();
  tc->SetCenter(x,y,z);
  tc->PreventSeamOn();
  tc->AutomaticSphereGenerationOff();
  tc->SetInputConnection(ret->s->GetOutputPort());
  vtkSmartPointer<vtkTransformTextureCoords> tt =
    vtkSmartPointer<vtkTransformTextureCoords>::New();
  tt->SetInputConnection(tc->GetOutputPort());
  //tt->SetScale(1,0.5,1);
  //make normals
  vtkSmartPointer<vtkPolyDataNormals> nl =
    vtkSmartPointer<vtkPolyDataNormals>::New();
  nl->SetInputConnection(tt->GetOutputPort());
  nl->Update();
  //make more attribute arrays
  vtkPolyData *pd = nl->GetOutput();
  //point aligned
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName("testarray1");
  da->SetNumberOfComponents(1);
  pd->GetPointData()->AddArray(da);
  int np = pd->GetNumberOfPoints();
  int nc = pd->GetNumberOfCells();
  for (int i = 0; i < np; i++)
  {
    da->InsertNextValue((double)i/(double)np);
  }
  da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName("testarray2");
  da->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(da);
  for (int i = 0; i < np; i++)
  {
    double vals[3] = {(double)i/(double)np, (double)(i*4)/(double)np-2.0, 42.0};
    da->InsertNextTuple3(vals[0],vals[1],vals[2]);
  }

  vtkSmartPointer<vtkUnsignedCharArray> pac =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  pac->SetName("testarrayc1");
  pac->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(pac);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] =
      {static_cast<unsigned char>(255*((double)i/(double)np)),
       static_cast<unsigned char>(255*((double)(i*4)/(double)np-2.0)),
       42};
    pac->InsertNextTuple3(vals[0],vals[1],vals[2]);
  }

  vtkSmartPointer<vtkUnsignedCharArray> ca =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  ca->SetName("testarray3");
  ca->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(ca);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] =
      {static_cast<unsigned char>((double)i/(double)np*255),
       static_cast<unsigned char>((double)(1-i)/(double)np),
       42};
    ca->InsertNextTuple3(vals[0],vals[1],vals[2]);
  }
  //cell aligned
  da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName("testarray4");
  da->SetNumberOfComponents(1);
  pd->GetCellData()->AddArray(da);
  for (int i = 0; i < pd->GetNumberOfCells(); i++)
  {
    da->InsertNextValue((double)i/(double)pd->GetNumberOfCells());
  }
  da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName("testarray5");
  da->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(da);
  for (int i = 0; i < nc; i++)
  {
    double vals[3] = {(double)i/(double)nc, (double)(i*2)/(double)nc, 42.0};
    da->InsertNextTuple3(vals[0],vals[1],vals[2]);
  }
  ca = vtkSmartPointer<vtkUnsignedCharArray>::New();
  ca->SetName("testarray6");
  ca->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(ca);
  for (int i = 0; i < nc; i++)
  {
    unsigned char vals[3] =
      {static_cast<unsigned char>((double)i/(double)np*255),
       static_cast<unsigned char>((double)(1-i)/(double)np),
       42};
    ca->InsertNextTuple3(vals[0],vals[1],vals[2]);
  }
  ret->m = vtkPolyDataMapper::New();
  ret->m->SetInputData(pd);

  switch (type)
  {
    case 0: //points
    {
      vtkSmartPointer<vtkVertexGlyphFilter> filter =
        vtkSmartPointer<vtkVertexGlyphFilter>::New();
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
    case 1: //lines
    {
      vtkSmartPointer<vtkExtractEdges> filter =
        vtkSmartPointer<vtkExtractEdges>::New();
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
    case 2: //polys
      break;
    case 3: //strips
    {
      vtkSmartPointer<vtkStripper> filter =
        vtkSmartPointer<vtkStripper>::New();
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
  }
  ret->a=vtkActor::New();
  ret->a->SetMapper(ret->m);
  ret->a->GetProperty()->SetPointSize(20);
  ret->a->GetProperty()->SetLineWidth(10);
  if (rep != -1)
  {
    ret->a->GetProperty()->SetRepresentation(rep);
  }
  return ret;
}

int TestOSPRayRenderMesh(int argc, char* argv[])
{
  bool useGL = false;
  int type = 2;
  int rep = -1;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      useGL = true;
    }
    if (!strcmp(argv[i], "-type"))
    {
      type = atoi(argv[i+1]);
    }
    if (!strcmp(argv[i], "-rep"))
    {
      rep = atoi(argv[i+1]);
    }
  }

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.75,0.75,0.75);
  renWin->SetSize(600,550);
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(2.5,11,-3);
  camera->SetFocalPoint(2.5,0,-3);
  camera->SetViewUp(0,0,1);
  renderer->SetActiveCamera(camera);
  renWin->Render();

  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  if (!useGL)
  {
    renderer->SetPass(ospray);
  }
  //Now, vary of most of the many parameters that rendering can vary by.

  //representations points, wireframe, surface
  renderable *ren = MakeSphereAt(5,0,-5, 10, type, rep, "points");
  ren->a->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(ren->a);
  delete(ren);

  ren = MakeSphereAt(5,0,-4, 10, type, rep, "wireframe");
  ren->a->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(ren->a);
  delete(ren);

  ren = MakeSphereAt(5,0,-3, 10, type, rep, "surface");
  ren->a->GetProperty()->SetRepresentationToSurface();
  renderer->AddActor(ren->a);
  delete(ren);

  //actor color
  ren = MakeSphereAt(4,0,-5, 10, type, rep, "actor_color");
  ren->a->GetProperty()->SetColor(0,1,0);
  renderer->AddActor(ren->a);
  delete(ren);

  //ambient, diffuse, and specular components
  ren = MakeSphereAt(4,0,-4, 7, type, rep, "amb/diff/spec");
  ren->a->GetProperty()->SetAmbient(0.5);
  ren->a->GetProperty()->SetAmbientColor(0.1,0.1,0.3);
  ren->a->GetProperty()->SetDiffuse(0.4);
  ren->a->GetProperty()->SetDiffuseColor(0.5,0.1,0.1);
  ren->a->GetProperty()->SetSpecular(0.2);
  ren->a->GetProperty()->SetSpecularColor(1,1,1);
  ren->a->GetProperty()->SetSpecularPower(100);
  ren->a->GetProperty()->SetInterpolationToPhong();
  renderer->AddActor(ren->a);
  delete(ren);

  //opacity
  ren = MakeSphereAt(4,0,-3, 10, type, rep, "opacity");
  ren->a->GetProperty()->SetOpacity(0.2);
  renderer->AddActor(ren->a);
  delete(ren);

  //color map cell values
  ren = MakeSphereAt(3,0,-5, 10, type, rep, "cell_value");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(0);
  renderer->AddActor(ren->a);
  delete(ren);

  //default color component
  ren = MakeSphereAt(3,0,-4, 10, type, rep, "cell_default_comp");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(1);
  renderer->AddActor(ren->a);
  delete(ren);

  //choose color component
  ren = MakeSphereAt(3,0,-3, 10, type, rep, "cell_comp_1");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(1);
  ren->m->ColorByArrayComponent(1,1); //todo, use lut since this is deprecated
  renderer->AddActor(ren->a);
  delete(ren);

  //RGB direct
  ren = MakeSphereAt(3,0,-2, 10, type, rep, "cell_rgb");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(2);
  renderer->AddActor(ren->a);
  delete(ren);

  //RGB through LUT
  ren = MakeSphereAt(3,0,-1, 10, type, rep, "cell_rgb_through_LUT");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(2);
  ren->m->SetColorModeToMapScalars();
  renderer->AddActor(ren->a);
  delete(ren);

  //color map point values
  ren = MakeSphereAt(2,0,-5,6, type, rep, "point_value");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SelectColorArray("testarray1");
  renderer->AddActor(ren->a);
  delete(ren);

  //interpolate scalars before mapping
  ren = MakeSphereAt(2,0,-4,6, type, rep, "point_interp");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SelectColorArray("testarray1");
  ren->m->InterpolateScalarsBeforeMappingOn();
  renderer->AddActor(ren->a);
  delete(ren);

  //RGB direct
  ren = MakeSphereAt(2,0,-3, 10, type, rep, "point_rgb");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SetColorModeToDefault();
  ren->m->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->a);
  delete(ren);

  //RGB mapped
  ren = MakeSphereAt(2,0,-2, 10, type, rep, "point_rgb_through_LUT");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SetColorModeToMapScalars();
  ren->m->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->a);
  delete(ren);

  //unlit, flat, and gouraud lighting
  ren = MakeSphereAt(1,0,-5,7, type, rep, "not_lit");
  ren->a->GetProperty()->LightingOff();
  renderer->AddActor(ren->a);
  delete(ren);

  ren = MakeSphereAt(1,0,-4,7, type, rep, "flat");
  ren->a->GetProperty()->SetInterpolationToFlat();
  renderer->AddActor(ren->a);
  delete(ren);

  ren = MakeSphereAt(1,0,-3,7, type, rep, "gouraud");
  ren->a->GetProperty()->SetInterpolationToGouraud();
  renderer->AddActor(ren->a);
  delete(ren);

  //texture
  int maxi = 100;
  int maxj = 100;
  vtkSmartPointer<vtkImageData> texin = vtkSmartPointer<vtkImageData>::New();
  texin->SetExtent(0,maxi,0,maxj,0,0);
  texin->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray *aa =
    vtkArrayDownCast<vtkUnsignedCharArray>(texin->GetPointData()->GetScalars());
  int idx = 0;
  for (int i = 0; i<=maxi; i++)
  {
    for (int j = 0; j<=maxj; j++)
    {
      bool ival = (i/10)%2==1;
      bool jval = (j/10)%2==1;
      unsigned char val = (ival^jval) ? 255 : 0;
      aa->SetTuple3(idx, val, val, val);
      if (j <= 3 || j >= maxj-3)
      {
        aa->SetTuple3(idx, 255, 255, 0);
      }
      if (i <= 20 || i >= maxi-20)
      {
        aa->SetTuple3(idx, 255, 0, 0);
      }
      idx = idx + 1;
    }
  }
  ren = MakeSphereAt(0,0,-5,20,type, rep, "texture");
  renderer->AddActor(ren->a);
  vtkSmartPointer<vtkTexture> texture =
    vtkSmartPointer<vtkTexture>::New();
  texture->SetInputData(texin);
  ren->a->SetTexture(texture);
  delete(ren);

  //imagespace positional transformations
  ren = MakeSphereAt(0,0,-4,10, type, rep, "transform");
  ren->a->SetScale(1.2,1.0,0.87);
  renderer->AddActor(ren->a);
  delete(ren);

  //TODO: lut manipulation and range effects
  //TODO: NaN colors
  //TODO: mapper clipping planes
  //TODO: hierarchical actors

  renWin->Render();

  vtkLight *light =
    vtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  double lColor[3];
  light->GetDiffuseColor(lColor);
  light->SetPosition(2,15,-2);
  light->SetFocalPoint(2,0,-2);
  light->PositionalOff();

  vtkSmartPointer<vtkOSPRayTestInteractor> style =
    vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->
    SetPipelineControlPoints((vtkOpenGLRenderer*)renderer.Get(), ospray, NULL);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
