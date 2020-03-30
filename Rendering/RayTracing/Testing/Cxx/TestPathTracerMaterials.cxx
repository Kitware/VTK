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
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayTestInteractor.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSuperquadricSource.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"

// !! NOTE this test will output different images based on the OSPRay version,
// !! since the available materials changed with OSPRay v1.6.

int TestPathTracerMaterials(int argc, char* argv[])
{
  // set up the environment
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  vtkOSPRayRendererNode::SetSamplesPerPixel(1, renderer);
  vtkOSPRayRendererNode::SetBackgroundMode(2, renderer);
  renWin->SetSize(700, 700);
  double up[3] = { 0., 1., 0. };
  vtkOSPRayRendererNode::SetNorthPole(up, renderer);
  double east[3] = { 1., 0., 0. };
  vtkOSPRayRendererNode::SetEastPole(east, renderer);
  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  // todo: let user switch between PT and SV renderer
  vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  // use an environment map so that materials have something to reflect
  vtkSmartPointer<vtkTexture> textr = vtkSmartPointer<vtkTexture>::New();
  vtkSmartPointer<vtkJPEGReader> imgReader = vtkSmartPointer<vtkJPEGReader>::New();
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wintersun.jpg");
  imgReader->SetFileName(fname);
  delete[] fname;
  imgReader->Update();
  textr->SetInputConnection(imgReader->GetOutputPort(0));
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(textr);
  vtkSmartPointer<vtkOSPRayTestInteractor> style = vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // make some predictable data to test with
  // anything will do, but should have normals and textures coordinates
  // for materials to work with
  vtkSmartPointer<vtkSuperquadricSource> polysource = vtkSmartPointer<vtkSuperquadricSource>::New();
  polysource->ToroidalOn(); // mmmmm ... daddy's soul donut
  polysource->SetThetaResolution(50);
  polysource->SetPhiResolution(50);
  // measure it so we can automate positioning
  polysource->Update();
  double* bds = polysource->GetOutput()->GetBounds();
  double xo = bds[0];
  double xr = bds[1] - bds[0];
  double yo = bds[2];
  // double yr = bds[1]-bds[0];
  double zo = bds[4];
  double zr = bds[1] - bds[0];

  // make a predictable texture too
  int maxi = 100;
  int maxj = 100;
  vtkSmartPointer<vtkImageData> texin = vtkSmartPointer<vtkImageData>::New();
  texin->SetExtent(0, maxi, 0, maxj, 0, 0);
  texin->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray* aa =
    vtkArrayDownCast<vtkUnsignedCharArray>(texin->GetPointData()->GetScalars());
  int idx = 0;
  for (int _i = 0; _i <= maxi; _i++)
  {
    for (int _j = 0; _j <= maxj; _j++)
    {
      bool ival = (_i / 10) % 2 == 1;
      bool jval = (_j / 10) % 2 == 1;
      unsigned char val = (ival ^ jval) ? 255 : 0;
      aa->SetTuple3(idx, val, val, val);
      if (val == 255)
      {
        aa->SetTuple3(idx, val, 0, 0);
      }
      else
      {
        aa->SetTuple3(idx, 0, val, 0);
      }
      if (_j <= 3 || _j >= maxj - 3)
      {
        aa->SetTuple3(idx, 127, 127, 0);
      }
      if (_i <= 20 || _i >= maxi - 20)
      {
        aa->SetTuple3(idx, 0, 127, 127);
      }
      idx = idx + 1;
    }
  }
  vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
  texture->SetInputData(texin);

  // now what we actually want to test.
  // draw the data at different places
  // varying the visual characteristics each time

  int i, j = 0;
  vtkSmartPointer<vtkActor> actor;
  vtkProperty* prop;
  vtkSmartPointer<vtkPolyDataMapper> mapper;

  // get a hold of the material library
  vtkSmartPointer<vtkOSPRayMaterialLibrary> ml = vtkSmartPointer<vtkOSPRayMaterialLibrary>::New();
  vtkOSPRayRendererNode::SetMaterialLibrary(ml, renderer);

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // plain old color, no fancy shmancy material here by gum
  i = 0;
  j = 0;
  {
    style->AddName("actor color");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  // color mapping, this is VTK after all
  j++;
  {
    style->AddName("color mapping");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkPolyData> copy = vtkSmartPointer<vtkPolyData>::New();
    copy->ShallowCopy(polysource->GetOutput());
    mapper->SetInputData(copy);
    vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
    da->SetNumberOfComponents(0);
    da->SetName("test_array");
    for (int c = 0; c < copy->GetNumberOfCells(); ++c)
    {
      da->InsertNextValue(c / (double)copy->GetNumberOfCells());
    }
    copy->GetCellData()->SetScalars(da);
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  // invalid material, should warn but draw normal material
  j++;
  {
    style->AddName("invalid material");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("flubber"); // test not supported mat name
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // glass
  i = 1;
  j = 0;
  {
    style->AddName("default glass");

    ml->AddMaterial("Glass 1", "Glass");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    double eta[1] = { 2.0 };
    ml->AddShaderVariable("Glass 2", "etaInside", 1, eta);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    double green[3] = { 0.0, 1.0, 0.0 };
    ml->AddShaderVariable("Glass 3", "attenuationColor", 3, green);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
  // metal
  i = 2;
  j = 0;
  {
    style->AddName("default metal");

    ml->AddMaterial("Metal 1", "Metal");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    double roughness[1] = { 0.0 };
    ml->AddShaderVariable("Metal 3", "roughness", 1, roughness);
    // for OSP < 1.4 and > 1.4.0
    double cuColor[3] = { 0.7843, 0.4588, 0.2 };
    ml->AddShaderVariable("Metal 3", "reflectance", 3, cuColor);
    // for OSP >= 1.4
    double spectrum[58 * 3] = {
      300, 1.347459987, 1.679419071, //
      310, 1.321473211, 1.740141215, //
      320, 1.301896917, 1.781554261, //
      330, 1.278815346, 1.816251273, //
      340, 1.257856058, 1.857525737, //
      350, 1.229714372, 1.895968733, //
      360, 1.205793784, 1.941169403, //
      370, 1.183134074, 1.99326522,  //
      380, 1.16577487, 2.046321345,  //
      390, 1.139929606, 2.090129064, //
      400, 1.119339006, 2.14224644,  //
      410, 1.097661459, 2.193481406, //
      420, 1.082884327, 2.251163803, //
      430, 1.067185209, 2.306769228, //
      440, 1.056310845, 2.361946782, //
      450, 1.048210496, 2.413637347, //
      460, 1.044058354, 2.464134299, //
      470, 1.040826414, 2.50896784,  //
      480, 1.040383818, 2.549587906, //
      490, 1.035622719, 2.577676166, //
      500, 1.0292166, 2.600958825,   //
      510, 1.01596237, 2.610628188,  //
      520, 0.995463808, 2.613856957, //
      530, 0.957525814, 2.60358516,  //
      540, 0.896412084, 2.584135179, //
      550, 0.79745994, 2.56420404,   //
      560, 0.649913539, 2.566649101, //
      570, 0.467667795, 2.633707115, //
      580, 0.308052581, 2.774526337, //
      590, 0.206477543, 2.953105649, //
      600, 0.15342929, 3.124794481,  //
      610, 0.129738592, 3.28082796,  //
      620, 0.116677068, 3.422223479, //
      630, 0.110069919, 3.546563885, //
      640, 0.107194012, 3.666809315, //
      650, 0.104232496, 3.775693898, //
      660, 0.102539467, 3.879628119, //
      670, 0.102449402, 3.981770445, //
      680, 0.101216009, 4.082308744, //
      690, 0.101603953, 4.175083635, //
      700, 0.101236908, 4.27062629,  //
      710, 0.101557633, 4.365353818, //
      720, 0.101132194, 4.453675754, //
      730, 0.100848965, 4.541494304, //
      740, 0.100919789, 4.632837662, //
      750, 0.101173963, 4.718605321, //
      760, 0.101837799, 4.806908667, //
      770, 0.101672055, 4.890330992, //
      780, 0.104166566, 4.985764803, //
      790, 0.10154611, 5.058785587,  //
      800, 0.105089997, 5.141307607, //
      810, 0.105640925, 5.225721003, //
      820, 0.1047717, 5.314412207,   //
      830, 0.108065424, 5.399044187, //
      840, 0.106329275, 5.471682183, //
      850, 0.106803015, 5.558363688, //
      860, 0.10806138, 5.64355183,   //
      870, 0.109423947, 5.718126756  //
    };
    ml->AddShaderVariable("Metal 3", "ior", 58 * 3, spectrum);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
  // OBJMaterial
  i++;
  j = 0;
  {
    style->AddName("default objmaterial");

    ml->AddMaterial("OBJMaterial 1", "OBJMaterial");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  { // todo: validate
    style->AddName("alpha objmaterial");

    ml->AddMaterial("OBJMaterial 2", "OBJMaterial");
    double alpha[1] = { 0.2 };
    ml->AddShaderVariable("OBJMaterial 2", "alpha", 1, alpha);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
  j = 0;
  { // todo: looks 'approximately' right but needs validation
    style->AddName("bump map objmaterial");

    ml->AddMaterial("OBJMaterial 7", "OBJMaterial");
    ml->AddTexture("OBJMaterial 7", "map_bump", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("OBJMaterial 8");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  { // todo: validate
    style->AddName("kd map objmaterial");

    ml->AddMaterial("OBJMaterial 9", "OBJMaterial");
    ml->AddTexture("OBJMaterial 9", "map_kd", texture);

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
  // thin glass
  i++;
  j = 0;
  {
    style->AddName("default thin glass");

    ml->AddMaterial("ThinGlass 1", "ThinGlass");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
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
  // CarPaint
  i++;
  j = 0;
  {
    style->AddName("default car paint");

    ml->AddMaterial("CarPaint 1", "CarPaint");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("CarPaint 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("Flakey purpley pink car paint");

    ml->AddMaterial("CarPaint 2", "CarPaint");
    ml->AddShaderVariable("CarPaint 2", "baseColor", { 1., .1, .9 });
    ml->AddShaderVariable("CarPaint 2", "coatColor", { 1., .1, .9 });
    ml->AddShaderVariable("CarPaint 2", "flipflopColor", { .5, .1, .9 });
    ml->AddShaderVariable("CarPaint 2", "flipflopFalloff", { .2 });
    ml->AddShaderVariable("CarPaint 2", "flakeDensity", { .9 });
    ml->AddShaderVariable("CarPaint 2", "flakeSpread", { .5 });
    ml->AddShaderVariable("CarPaint 2", "flakeScale", { 500. });
    ml->AddShaderVariable("CarPaint 2", "flakeRoughness", { .75 });

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("CarPaint 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("Silvery blue car paint");

    ml->AddMaterial("CarPaint 3", "CarPaint");
    ml->AddShaderVariable("CarPaint 3", "baseColor", { .4, .4, .6 });
    ml->AddShaderVariable("CarPaint 3", "coatColor", { .7, .7, .8 });
    ml->AddShaderVariable("CarPaint 3", "coatThickness", { .3 });
    ml->AddShaderVariable("CarPaint 3", "flipflopColor", { .1, .2, .95 });
    ml->AddShaderVariable("CarPaint 3", "flipflopFalloff", { .1 });
    ml->AddShaderVariable("CarPaint 3", "flakeDensity", { .4 });
    ml->AddShaderVariable("CarPaint 3", "flakeSpread", { .2 });
    ml->AddShaderVariable("CarPaint 3", "flakeScale", { 50. });
    ml->AddShaderVariable("CarPaint 3", "flakeRoughness", { .8 });

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("CarPaint 3");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Principled
  i++;
  j = 0;
  {
    style->AddName("default principled");

    ml->AddMaterial("Principled 1", "Principled");

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Principled 1");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  j++;
  {
    style->AddName("green sheen principled");

    ml->AddMaterial("Principled 2", "Principled");
    ml->AddShaderVariable("Principled 2", "baseColor", { .6, .9, .6 });
    ml->AddShaderVariable("Principled 2", "edgeColor", { .1, .9, .2 });
    ml->AddShaderVariable("Principled 2", "metallic", { .7 });
    ml->AddShaderVariable("Principled 2", "sheenColor", { .2, .9, .1 });
    ml->AddShaderVariable("Principled 2", "sheen", { .9 });
    ml->AddShaderVariable("Principled 2", "coatColor", { .2, .9, .1 });

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor->GetProperty();
    prop->SetMaterialName("Principled 2");
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(polysource->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  // now finally draw
  renWin->Render();                           // let vtk pick a decent camera
  renderer->GetActiveCamera()->Elevation(30); // adjust to show more
  renWin->Render();

  // hook up ability to focus on each object as RenderMesh test does
  style->SetPipelineControlPoints(renderer, ospray, nullptr);

  // set up progressive rendering
  vtkCommand* looper = style->GetLooper(renWin);
  vtkCamera* cam = renderer->GetActiveCamera();
  iren->AddObserver(vtkCommand::KeyPressEvent, looper);
  cam->AddObserver(vtkCommand::ModifiedEvent, looper);
  iren->CreateRepeatingTimer(10); // every 10 msec we'll rerender if needed
  iren->AddObserver(vtkCommand::TimerEvent, looper);

  // todo: use standard vtk testing conventions
  iren->Start();
  return 0;
}
