/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestRIB.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"

#include "vtkTestUtilities.h"
#include "vtkNamedColors.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTexture.h"
#include "vtkPNMReader.h"
#include "vtkSphereSource.h"
#include "vtkTexturedSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRIBExporter.h"
#include "vtkRIBProperty.h"

static vtkSmartPointer<vtkRIBProperty> cloth(const char *freq,
                                             const char *depth);
static vtkSmartPointer<vtkRIBProperty> dented(const char *Km);
static vtkSmartPointer<vtkRIBProperty> stippled(const char *grainsize,
                                                const char *stippling);
static vtkSmartPointer<vtkRIBProperty> spatter(const char *sizes,
                                               const char *specksize,
                                               const char *spattercolor,
                                               const char *basecolor);
static vtkSmartPointer<vtkRIBProperty> cmarble(const char *veining);
static vtkSmartPointer<vtkRIBProperty> stone(const char *scale,
                                             const char *nshades,
                                             const char *exponent,
                                             const char *graincolor);
static vtkSmartPointer<vtkRIBProperty> wood(const char *grain,
                                            const char *swirl,
                                            const char *swirlfreq);
static vtkSmartPointer<vtkRIBProperty> bozo(const char *k);

int TestRIBExporter (int argc, char *argv[])
{
  const char *_prefix = vtkTestUtilities::GetArgOrEnvOrDefault(
    "-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string prefix = _prefix;
  delete []_prefix;

  if (prefix == "")
  {
    std::cout << argv[0] << " Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  prefix += "/TestRIBExporter";

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New ();
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New ();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New ();

  char *textureFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/earth.ppm");
  vtkSmartPointer<vtkPNMReader> textureReader =
    vtkSmartPointer<vtkPNMReader>::New ();
  textureReader->SetFileName(textureFile);
  delete []textureFile;

  vtkSmartPointer<vtkTexturedSphereSource> texturedSphere =
    vtkSmartPointer<vtkTexturedSphereSource>::New ();
  texturedSphere->SetPhiResolution(20);
  texturedSphere->SetThetaResolution(20);

  vtkSmartPointer<vtkTexture> texture =
    vtkSmartPointer<vtkTexture>::New ();
  texture->SetInputConnection(textureReader->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper>  texturedSphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New ();
  texturedSphereMapper->SetInputConnection(texturedSphere->GetOutputPort());

  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New ();
  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);

  vtkSmartPointer<vtkPolyDataMapper>  sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New ();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkActor> sphere1 =
    vtkSmartPointer<vtkActor>::New ();
  sphere1->SetMapper(sphereMapper);
  sphere1->SetPosition( -1.5, 1.5, 0);

  vtkSmartPointer<vtkActor> sphere2 =
    vtkSmartPointer<vtkActor>::New ();
  sphere2->SetMapper(sphereMapper);
  sphere2->SetPosition( 0, 1.5, 0);

  vtkSmartPointer<vtkActor> sphere3 =
    vtkSmartPointer<vtkActor>::New ();
  sphere3->SetMapper(sphereMapper);
  sphere3->SetPosition(1.5, 1.5, 0);

  vtkSmartPointer<vtkActor> sphere4 =
    vtkSmartPointer<vtkActor>::New ();
  sphere4->SetMapper(sphereMapper);
  sphere4->SetPosition(-1.5, 0, 0);

  vtkSmartPointer<vtkActor> sphere5 =
    vtkSmartPointer<vtkActor>::New ();
  sphere5->SetMapper(sphereMapper);
  sphere5->SetPosition(0, 0, 0);

  vtkSmartPointer<vtkActor> sphere6 =
    vtkSmartPointer<vtkActor>::New ();
  sphere6->SetMapper(sphereMapper);
  sphere6->SetPosition(1.5, 0, 0);

  vtkSmartPointer<vtkActor> sphere7 =
    vtkSmartPointer<vtkActor>::New ();
  sphere7->SetMapper(sphereMapper);
  sphere7->SetPosition(-1.5, -1.5, 0);

  vtkSmartPointer<vtkActor> sphere8 =
    vtkSmartPointer<vtkActor>::New ();
  sphere8->SetMapper(sphereMapper);
  sphere8->SetPosition(0, -1.5, 0);

  vtkSmartPointer<vtkActor> sphere9 =
    vtkSmartPointer<vtkActor>::New ();
  sphere9->SetMapper(texturedSphereMapper);
  sphere9->SetTexture(texture);
  sphere9->SetPosition(1.5,-1.5, 0);
  sphere9->SetOrientation(90, 0, 0);

  renWin->AddRenderer(ren1);
  iren->SetRenderWindow(renWin);

  ren1->AddActor(sphere1);
  ren1->AddActor(sphere2);
  ren1->AddActor(sphere3);
  ren1->AddActor(sphere4);
  ren1->AddActor(sphere5);
  ren1->AddActor(sphere6);
  ren1->AddActor(sphere7);
  ren1->AddActor(sphere8);
  ren1->AddActor(sphere9);

  ren1->SetBackground(0.10, 0.2, 0.4);
  renWin->SetSize(640, 480);

  vtkSmartPointer<vtkNamedColors> colors =
    vtkSmartPointer<vtkNamedColors>::New();
  double color[4];

  sphere1->SetProperty(cloth("500", ".02"));
  colors->GetColor("gold", color);
  sphere1->GetProperty()->SetDiffuseColor(color);

  sphere2->SetProperty(stippled(".1", "1"));
  colors->GetColor("ivory", color);
  sphere2->GetProperty()->SetDiffuseColor(color);

  sphere3->SetProperty(spatter("5", ".5", "0 0 0", "1 1 1"));

  sphere4->SetProperty(cmarble("4"));

  sphere5->SetProperty(stone(".07", "2", "2", ".2 .3 .4"));

  sphere6->SetProperty(wood("5", ".25", "1"));
  sphere7->SetProperty(bozo("5"));

  sphere8->SetProperty(dented("5"));
  colors->GetColor("tomato", color);
  sphere8->GetProperty()->SetDiffuse(.7);
  sphere8->GetProperty()->SetDiffuseColor(color);
  sphere8->GetProperty()->SetSpecular(.5);
  sphere8->GetProperty()->SetSpecularPower(5);

  vtkCamera *cam1 = ren1->GetActiveCamera();
  ren1->ResetCamera();

  cam1->Zoom(1.5);

  renWin->Render();

  vtkSmartPointer<vtkRIBExporter> aRib =
    vtkSmartPointer<vtkRIBExporter>::New ();
  aRib->SetInput(renWin);
  aRib->SetFilePrefix(prefix.c_str());
  aRib->SetTexturePrefix(prefix.c_str());
  aRib->BackgroundOn();
  aRib->Write();

  return EXIT_SUCCESS;
}

vtkSmartPointer<vtkRIBProperty> cloth(const char *freq, const char *depth)
{
  vtkSmartPointer<vtkRIBProperty> clothProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  clothProp->SetVariable("freq", "float");
  clothProp->AddVariable("depth", "float");

  clothProp->SetDisplacementShaderParameter("freq", freq);
  clothProp->AddDisplacementShaderParameter("depth", depth);

  clothProp->SetDisplacementShader("cloth");
  return clothProp;
}

vtkSmartPointer<vtkRIBProperty> dented(const char *Km)
{
  vtkSmartPointer<vtkRIBProperty> dentedProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  dentedProp->SetVariable("Km", "float");
  dentedProp->SetDisplacementShaderParameter("Km", Km);
  dentedProp->SetDisplacementShader("dented");
  return dentedProp;
}

vtkSmartPointer<vtkRIBProperty> stippled(const char *grainsize, const char *stippling)
{
  vtkSmartPointer<vtkRIBProperty> stippledProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  stippledProp->SetVariable("grainsize", "float");
  stippledProp->AddVariable("stippling", "float");

  stippledProp->SetSurfaceShaderParameter("grainsize", grainsize);
  stippledProp->AddSurfaceShaderParameter("stippling", stippling);

  stippledProp->SetSurfaceShader("stippled");
  return stippledProp;
}

vtkSmartPointer<vtkRIBProperty> bozo(const char *k)
{
  vtkSmartPointer<vtkRIBProperty> bozoProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  bozoProp->SetSurfaceShader("bozo");
  bozoProp->SetVariable("k", "float");
  bozoProp->SetSurfaceShaderParameter("k", k);

  return bozoProp;
}

vtkSmartPointer<vtkRIBProperty> spatter(const char *sizes,
                                        const char *specksize,
                                        const char *spattercolor,
                                        const char *basecolor)
{
  vtkSmartPointer<vtkRIBProperty> spatterProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  spatterProp->SetVariable("sizes", "float");
  spatterProp->AddVariable("specksize", "float");
  spatterProp->AddVariable("spattercolor", "color");
  spatterProp->AddVariable("basecolor", "color");

  spatterProp->SetSurfaceShaderParameter("sizes", sizes);
  spatterProp->AddSurfaceShaderParameter("specksize", specksize);
  spatterProp->AddSurfaceShaderParameter("spattercolor", spattercolor);
  spatterProp->AddSurfaceShaderParameter("basecolor", basecolor);
  spatterProp->SetSurfaceShader("spatter");

  return spatterProp;
}
vtkSmartPointer<vtkRIBProperty> cmarble(const char *veining)
{
  vtkSmartPointer<vtkRIBProperty> cmarbleProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  cmarbleProp->SetVariable("veining", "float");
  cmarbleProp->SetSurfaceShaderParameter("veining", veining);
  cmarbleProp->SetSurfaceShader("cmarble");

  return cmarbleProp;
}
vtkSmartPointer<vtkRIBProperty> stone(const char *scale,
                                      const char *nshades,
                                      const char *exponent,
                                      const char *graincolor)
{
  vtkSmartPointer<vtkRIBProperty> stoneProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  stoneProp->SetVariable("scale", "float");
  stoneProp->AddVariable("nshades", "float");
  stoneProp->AddVariable("exponent", "float");
  stoneProp->AddVariable("graincolor", "color");

  stoneProp->SetSurfaceShaderParameter("scale", scale);
  stoneProp->AddSurfaceShaderParameter("nshades", nshades);
  stoneProp->AddSurfaceShaderParameter("exponent", exponent);
  stoneProp->AddSurfaceShaderParameter("graincolor", graincolor);

  stoneProp->SetSurfaceShader("stone");

  return stoneProp;
}

vtkSmartPointer<vtkRIBProperty> wood(const char *grain,
                                      const char *swirl,
                                     const char *swirlfreq)
{
  vtkSmartPointer<vtkRIBProperty> woodProp =
    vtkSmartPointer<vtkRIBProperty>::New ();
  woodProp->SetVariable("grain", "float");
  woodProp->AddVariable("swirl", "float");
  woodProp->AddVariable("swirlfreq", "float");

  woodProp->SetSurfaceShaderParameter("grain", grain);
  woodProp->AddSurfaceShaderParameter("swirl", swirl);
  woodProp->AddSurfaceShaderParameter("swirlfreq", swirlfreq);

  woodProp->SetSurfaceShader("wood");

  return woodProp;
}
