/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that actor level materials work with path tracer
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayTestInteractor.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSuperquadricSource.h"
#include "vtkTexture.h"

int TestPathTracerMaterials(int argc, char* argv[])
{
  // set up the environment
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  vtkOSPRayRendererNode::SetSamplesPerPixel(1, renderer);
  renWin->SetSize(1000,1000);
  double up[3] = {0.,1.,0.};
  vtkOSPRayRendererNode::SetNorthPole(up, renderer);
  double east[3] = {1.,0.,0.};
  vtkOSPRayRendererNode::SetEastPole(east, renderer);
  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  //todo: let user switch between PT and SV renderer
  vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);

  //use an environment map so that materials have something to reflect
  vtkSmartPointer<vtkTexture> textr = vtkSmartPointer<vtkTexture>::New();
  vtkSmartPointer<vtkJPEGReader> imgReader = vtkSmartPointer<vtkJPEGReader>::New();
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  char* fname = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/wintersun.jpg");
  imgReader->SetFileName(fname);
  delete [] fname;
  imgReader->Update();
  textr->SetInputConnection(imgReader->GetOutputPort(0));
  renderer->TexturedBackgroundOn();
  renderer->SetBackgroundTexture(textr);
  vtkSmartPointer<vtkOSPRayTestInteractor> style =
    vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  //make some predictable data to test with
  //anything will do, but should have normals and textures coordinates
  //for materials to work with
  vtkSmartPointer<vtkSuperquadricSource> polysource =
    vtkSmartPointer<vtkSuperquadricSource>::New();
  polysource->ToroidalOn(); //mmmmm ... daddy's soul donut
  polysource->SetThetaResolution(50);
  polysource->SetPhiResolution(50);
  //measure it so we can automate positioning
  polysource->Update();
  double *bds = polysource->GetOutput()->GetBounds();
  double xo = bds[0];
  double xr = bds[1]-bds[0];
  double yo = bds[2];
  //double yr = bds[1]-bds[0];
  double zo = bds[4];
  double zr = bds[1]-bds[0];

  //make a predictable texture too
  int maxi = 100;
  int maxj = 100;
  vtkSmartPointer<vtkImageData> texin = vtkSmartPointer<vtkImageData>::New();
  texin->SetExtent(0,maxi,0,maxj,0,0);
  texin->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray *aa =
  vtkArrayDownCast<vtkUnsignedCharArray>(texin->GetPointData()->GetScalars());
  int idx = 0;
  for (int _i = 0; _i<=maxi; _i++)
  {
    for (int _j = 0; _j<=maxj; _j++)
    {
      bool ival = (_i/10)%2==1;
      bool jval = (_j/10)%2==1;
      unsigned char val = (ival^jval) ? 255 : 0;
      aa->SetTuple3(idx, val, val, val);
      if (val == 255)
      {
        aa->SetTuple3(idx, val, 0, 0);
      }
      else
      {
        aa->SetTuple3(idx, 0, val, 0);
      }
      if (_j <= 3 || _j >= maxj-3)
      {
        aa->SetTuple3(idx, 127, 127, 0);
      }
      if (_i <= 20 || _i >= maxi-20)
      {
        aa->SetTuple3(idx, 0, 127, 127);
      }
      idx = idx + 1;
    }
  }
  vtkSmartPointer<vtkTexture> texture =
  vtkSmartPointer<vtkTexture>::New();
  texture->SetInputData(texin);

  //now what we actually want to test.
  //draw the data at different places
  //varying the visual characteristics each time

  int i, j = 0;
  vtkSmartPointer<vtkActor> actor;
  vtkProperty *prop;
  vtkSmartPointer<vtkPolyDataMapper> mapper;

  //get a hold of the material library
  vtkSmartPointer<vtkOSPRayMaterialLibrary> ml = vtkOSPRayMaterialLibrary::GetInstance();

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //plain old color, no fancy shmancy material here by gum
  i=0;
  j=0;
  {
    style->AddName("actor color");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //color mapping, this is VTK after all
  j++;
  {
    style->AddName("color mapping");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkPolyData> copy = vtkSmartPointer<vtkPolyData>::New();
    copy->ShallowCopy(polysource->GetOutput());
    mapper->SetInputData(copy);
    vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
    da->SetNumberOfComponents(0);
    da->SetName("test_array");
    for (int c = 0; c < copy->GetNumberOfCells(); ++c)
    {
      da->InsertNextValue(c/(double)copy->GetNumberOfCells());
    }
    copy->GetCellData()->SetScalars(da);
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //invalid material, should warn but draw normal material
  j++;
  {
    style->AddName("invalid material");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("flubber"); //test not supported mat name
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //glass
  i=1;
  j=0;
  {
    style->AddName("default glass");

    ml->AddMaterial("Glass 1", "Glass");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Glass 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("high index of refraction glass");

    ml->AddMaterial("Glass 2", "Glass");
    double eta[1] = {2.0};
    ml->AddShaderVariable("Glass 2", "etaInside", 1, eta);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Glass 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("colored glass");

    ml->AddMaterial("Glass 3", "Glass");
    double green[3] = {0.0,1.0,0.0};
    ml->AddShaderVariable("Glass 3", "attenuationColor", 3, green);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Glass 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  /*
  j++;
  {//todo: doesn't look right
    style->AddName("attenuated glass");

    ml->AddMaterial("Glass 4", "Glass");
    double green[3] = {0.0,1.0,0.0};
    ml->AddShaderVariable("Glass 4", "attenuationColor", 3, green);
    double att[1] = {0};
    ml->AddShaderVariable("Glass 4", "attenuationDistance", 1, att);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Glass 4");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //matte
  i=2;
  j=0;
  {
    style->AddName("default matte");

    ml->AddMaterial("Matte 1", "Matte");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Matte 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("colored matte");

    ml->AddMaterial("Matte 2", "Matte");
    double reflectance[3] = {0.0,0.0,0.7};
    ml->AddShaderVariable("Matte 2", "reflectance", 3, reflectance);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Matte 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //metal
  i=3;
  j=0;
  {
    style->AddName("default metal");

    ml->AddMaterial("Metal 1", "Metal");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Metal 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("rough metal");

    ml->AddMaterial("Metal 2", "Metal");
    double roughness[1] = { 0.3 };
    ml->AddShaderVariable("Metal 2", "roughness", 1, roughness);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Metal 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("copper metal");

    ml->AddMaterial("Metal 3", "Metal");
    double cuColor[3] = { 0.7843, 0.4588, 0.2 };
    ml->AddShaderVariable("Metal 3", "reflectance", 3, cuColor);
    double roughness[1] = { 0.0 };
    ml->AddShaderVariable("Metal 3", "roughness", 1, roughness);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Metal 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  /* todo: metallic paint doesn't appear to be working.
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //metallicPaint
  i++;
  j=0;
  {//todo: validate
    style->AddName("default metallicpaint");

    ml->AddMaterial("MetallicPaint 1", "MetallicPaint");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("MetallicPaint 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {//todo: validate
    style->AddName("glittery metallicpaint");

    ml->AddMaterial("MetallicPaint 2", "MetallicPaint");
    double glitterSpread[1] = { 0.5 };
    ml->AddShaderVariable("MetallicPaint 2", "glitterSpread", 1, glitterSpread);
    double glitterColor[3] = { 1.0, 0.0, 0.0 };
    ml->AddShaderVariable("MetallicPaint 2", "glitterColor", 3, glitterColor);
    double shadeColor[3] = { 0.1, 0.8, 0.8 };
    ml->AddShaderVariable("MetallicPaint 2", "shadeColor", 3, shadeColor);
    double eta[1] = {1.1};
    ml->AddShaderVariable("MetallicPaint 2", "eta", 1, eta);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("MetallicPaint 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //OBJMaterial
  i++;
  j=0;
  {
    style->AddName("default objmaterial");

    ml->AddMaterial("OBJMaterial 1", "OBJMaterial");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  { //todo: validate
    style->AddName("alpha objmaterial");

    ml->AddMaterial("OBJMaterial 2", "OBJMaterial");
    double alpha[1] = {0.2};
    ml->AddShaderVariable("OBJMaterial 2", "alpha", 1, alpha);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  /* todo: doesn't look right
  j++;
  { //todo: validate
    style->AddName("color objmaterial");

    ml->AddMaterial("OBJMaterial 3", "OBJMaterial");
    double color[3] = {0.5,0.0,1.0};
    ml->AddShaderVariable("OBJMaterial 3", "color", 1, color);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  /* todo: doesn't look right
  j++;
  { //todo: validate
    style->AddName("ks objmaterial");

    ml->AddMaterial("OBJMaterial 4", "OBJMaterial");
    double ks[3] = {0.5,0.0,1.0};
    ml->AddShaderVariable("OBJMaterial 4", "ks", 1, ks);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 4");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  /* todo: doesn't look right
  j++;
  { //todo: validate
    style->AddName("ns objmaterial");

    ml->AddMaterial("OBJMaterial 5", "OBJMaterial");
    double ns[1] = {0.5};
    ml->AddShaderVariable("OBJMaterial 5", "ns", 1, ns);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 5");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  /* todo: doesn't look right
  j++;
  { //todo: validate
    style->AddName("tf objmaterial");

    ml->AddMaterial("OBJMaterial 6", "OBJMaterial");
    double tf[3] = {0.5,0.0,1.0};
    ml->AddShaderVariable("OBJMaterial 6", "tf", 1, tf);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 6");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  i++;
  j=0;
  { //todo: looks 'approximately' right but needs validation
    style->AddName("bump map objmaterial");

    ml->AddMaterial("OBJMaterial 7", "OBJMaterial");
    ml->AddTexture("OBJMaterial 7", "map_bump", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 7");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("opacity map objmaterial");

    ml->AddMaterial("OBJMaterial 8", "OBJMaterial");
    ml->AddTexture("OBJMaterial 8", "map_d", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 8");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  { //todo: validate
    style->AddName("kd map objmaterial");

    ml->AddMaterial("OBJMaterial 9", "OBJMaterial");
    ml->AddTexture("OBJMaterial 9", "map_kd", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 9");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  /* todo: doesn't look right
  j++;
  { //todo: validate
    style->AddName("ks map objmaterial");

    ml->AddMaterial("OBJMaterial 10", "OBJMaterial");
    ml->AddTexture("OBJMaterial 10", "map_ks", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 10");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  /* todo: doesn't look right
  j++;
  { //todo: validate
    style->AddName("ns map objmaterial");

    ml->AddMaterial("OBJMaterial 11", "OBJMaterial");
    ml->AddTexture("OBJMaterial 11", "map_ns", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 11");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //plastic
  i++;
  j=0;
  {
    style->AddName("default plastic");

    ml->AddMaterial("Plastic 1", "Plastic");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Plastic 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j=1;
  {
    style->AddName("colored plastic");

    ml->AddMaterial("Plastic 2", "Plastic");
    double pigmentColor[3] = { 1.0, 1.0, 0.0 };
    ml->AddShaderVariable("Plastic 2", "pigmentColor", 3, pigmentColor);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Plastic 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("shiny plastic");

    ml->AddMaterial("Plastic 3", "Plastic");
    double pigmentColor[3] = { 1.0, 1.0, 0.0 };
    ml->AddShaderVariable("Plastic 3", "pigmentColor", 3, pigmentColor);
    double thickness[1] = { 0.0 };
    ml->AddShaderVariable("Plastic 3", "thickness", 1, thickness);
    double eta[1] = { 8.0 };
    ml->AddShaderVariable("Plastic 3", "eta", 1, eta);
    double roughness[1] = { 0.5 };
    ml->AddShaderVariable("Plastic 3", "roughness", 1, roughness);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Plastic 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("rough plastic");

    ml->AddMaterial("Plastic 4", "Plastic");
    double pigmentColor[3] = { 1.0, 1.0, 0.0 };
    ml->AddShaderVariable("Plastic 4", "pigmentColor", 3, pigmentColor);
    double thickness[1] = { 0.0 };
    ml->AddShaderVariable("Plastic 4", "thickness", 1, thickness);
    double eta[1] = { 8.0 };
    ml->AddShaderVariable("Plastic 4", "eta", 1, eta);
    double roughness[1] = { 0.9 };
    ml->AddShaderVariable("Plastic 4", "roughness", 1, roughness);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Plastic 4");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //thin glass
  i++;
  j=0;
  {
    style->AddName("default thin glass");

    ml->AddMaterial("ThinGlass 1", "ThinGlass");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("ThinGlass 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("shinier thin glass");

    ml->AddMaterial("ThinGlass 2", "ThinGlass");
    double eta[1] = { 5.5 };
    ml->AddShaderVariable("ThinGlass 2", "eta", 1, eta);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("ThinGlass 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("red thin glass");

    ml->AddMaterial("ThinGlass 3", "ThinGlass");
    double thickness[1] = { 0.000001 };
    ml->AddShaderVariable("ThinGlass 3", "thickness", 1, thickness);
    double pigmentColor[3] = { 1.0, 0.0, 0.0 };
    ml->AddShaderVariable("ThinGlass 3", "transmission", 3, pigmentColor);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("ThinGlass 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  /* todo: ospray devs likely removing this.
  j++;
  { //todo: looks no different
    style->AddName("thinnish glass");

    ml->AddMaterial("ThinGlass 4", "ThinGlass");
    double thickness[1] = { 0.1 };
    ml->AddShaderVariable("ThinGlass 4", "thickness", 1, thickness);
    double pigmentColor[3] = { 1.0, 0.0, 0.0 };
    ml->AddShaderVariable("ThinGlass 4", "transmission", 3, pigmentColor);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("ThinGlass 4");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }
  */

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //velvet
  i++;
  j=0;
  {
    style->AddName("default velvet");

    ml->AddMaterial("Velvet 1", "Velvet");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Velvet 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("scattercolor velvet");

    ml->AddMaterial("Velvet 2", "Velvet");
    double horizonScatteringColor[3] = { 1.0, 0.0, 1.0 };
    ml->AddShaderVariable("Velvet 2", "horizonScatteringColor", 3, horizonScatteringColor);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Velvet 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("blue velvet");

    ml->AddMaterial("Velvet 3", "Velvet");
    double horizonScatteringColor[3] = { 0.6, 0.6, 1.0 };
    ml->AddShaderVariable("Velvet 3", "horizonScatteringColor", 3, horizonScatteringColor);
    double reflectance[3] = { 0.3, 0.3, 0.6 };
    ml->AddShaderVariable("Velvet 3", "reflectance", 3, reflectance);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo+xr*1.15*i, yo, zo+zr*1.1*j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Velvet 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //now finally draw
  renWin->Render(); //let vtk pick a decent camera
  renderer->GetActiveCamera()->Elevation(30); //adjust to show more
  renWin->Render();

  //hook up ability to focus on each object as RenderMesh test does
  style->SetPipelineControlPoints((vtkOpenGLRenderer*)renderer.Get(), ospray, NULL);

  //set up progressive rendering
  vtkCommand *looper = style->GetLooper(renWin);
  vtkCamera *cam = renderer->GetActiveCamera();
  iren->AddObserver(vtkCommand::KeyPressEvent, looper);
  cam->AddObserver(vtkCommand::ModifiedEvent, looper);
  iren->CreateRepeatingTimer(10); //every 10 msec we'll rerender if needed
  iren->AddObserver(vtkCommand::TimerEvent, looper);

  //todo: use standard vtk testing conventions
  iren->Start();
  return 0;
}
