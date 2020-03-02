/*=========================================================================

 Program:   Visualization Toolkit

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCullerCollection.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkImageGridSource.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkPLYReader.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSkybox.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"

#include "vtkOpenGLFluidMapper.h"

#include <array>

// Define this to render blue color water, otherwise will render red (blood)
// color
#define BLUE_WATER

// Global variables for particle data
static vtkNew<vtkPoints> g_Points;
constexpr static double g_DragonPos[3]{ 2, -0.5, 3 };

constexpr static float g_ParticleRadius = 0.03f;

//-----------------------------------------------------------------------------
// Enable this for interactive demonstration
//#define INTERACTIVE_DEMO
#ifdef INTERACTIVE_DEMO
#include "TestFluidDemo.cxx"
#endif

//-----------------------------------------------------------------------------
int TestFluidMapper(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->UseSRGBColorSpaceOn();
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  //------------------------------------------------------------
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  vtkNew<vtkPolyDataMapper> dragonMapper;
  dragonMapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> dragon;
  dragon->SetMapper(dragonMapper);
  dragon->SetScale(20, 20, 20);
  dragon->SetPosition(g_DragonPos[0], g_DragonPos[1], g_DragonPos[2]);
  dragon->GetProperty()->SetDiffuseColor(0.780392, 0.568627, 0.113725);
  dragon->GetProperty()->SetSpecular(1.0);
  dragon->GetProperty()->SetSpecularPower(80.0);
  dragon->GetProperty()->SetDiffuse(0.7);
  renderer->AddActor(dragon);

  //------------------------------------------------------------
  vtkSmartPointer<vtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);
  vtkSmartPointer<vtkPBRPrefilterTexture> prefilter = renderer->GetEnvMapPrefiltered();
  prefilter->SetPrefilterSamples(64);
  prefilter->SetPrefilterSize(64);

  vtkNew<vtkOpenGLTexture> textureCubemap;
  textureCubemap->CubeMapOn();
  textureCubemap->UseSRGBColorSpaceOn();

  std::string pathSkybox[6] = { "Data/skybox/posx.jpg", "Data/skybox/negx.jpg",
    "Data/skybox/posy.jpg", "Data/skybox/negy.jpg", "Data/skybox/posz.jpg",
    "Data/skybox/negz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkJPEGReader> jpg;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, pathSkybox[i].c_str());
    jpg->SetFileName(fname);
    delete[] fname;
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(jpg->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    textureCubemap->SetInputConnection(i, flip->GetOutputPort());
  }

  renderer->SetEnvironmentTexture(textureCubemap);
  renderer->UseImageBasedLightingOn();

  vtkNew<vtkSkybox> skybox;
  skybox->SetTexture(textureCubemap);
  renderer->AddActor(skybox);

  //------------------------------------------------------------
  vtkNew<vtkImageGridSource> grid;
  grid->SetGridSpacing(32, 32, 0);
  grid->SetLineValue(0.2);
  grid->SetFillValue(1.0);

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToMapScalars();
  vtkNew<vtkLookupTable> lut;
  texture->SetLookupTable(lut);
  lut->SetSaturationRange(0.0, 0.0);
  lut->SetValueRange(0.0, 1.0);
  lut->SetTableRange(0.0, 1.0);
  lut->Build();
  texture->InterpolateOn();
  texture->RepeatOn();
  texture->MipmapOn();
  texture->SetInputConnection(grid->GetOutputPort(0));
  texture->UseSRGBColorSpaceOn();

  vtkNew<vtkPlaneSource> plane;
  plane->SetNormal(0.0, -1.0, 0.0);
  plane->SetOrigin(-15.0, 0.0, -15.0);
  plane->SetPoint1(15, 0, -15);
  plane->SetPoint2(-15, 0, 15);
  plane->Update();

  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkActor> texturedPlane;
  texturedPlane->SetMapper(planeMapper);
  texturedPlane->GetProperty()->SetBaseColorTexture(texture);
  texturedPlane->GetProperty()->SetInterpolationToPBR();
  texturedPlane->GetProperty()->SetMetallic(0.2);
  texturedPlane->GetProperty()->SetRoughness(0.1);

  renderer->AddActor(texturedPlane);
  //------------------------------------------------------------

  vtkNew<vtkPolyData> pointData;
  pointData->SetPoints(g_Points);

  vtkNew<vtkOpenGLFluidMapper> fluidMapper;
  fluidMapper->SetInputData(pointData);

#ifdef INTERACTIVE_DEMO
  setupInteractiveDemo(renderWindow, renderer, iren, pointData, dragon, fluidMapper);
#else
  renderWindow->SetSize(400, 400);
  const float spacing = 0.1f;
  for (int z = 0; z < 50; ++z)
  {
    for (int y = 0; y < 15; ++y)
    {
      for (int x = 0; x < 50; ++x)
      {
        g_Points->InsertNextPoint(static_cast<double>(x * spacing),
          static_cast<double>(y * spacing), static_cast<double>(z * spacing));
      }
    }
  }
#endif

  // Begin parameters turning for fluid mapper ==========>
  // For new dataset, we may need to do multiple times turning parameters until
  // we get a nice result Must set parameter is particle radius,

  // MUST SET PARAMETER ==========================
  // Set the radius of the rendered spheres to be 2 times larger than the actual
  // sphere radius This is necessary to fuse the gaps between particles and
  // obtain a smooth surface
  fluidMapper->SetParticleRadius(g_ParticleRadius * 3.0f);

  // Set the number of iterations to filter the depth surface
  // This is an optional parameter, default value is 3
  // Usually set this to around 3-5
  // Too many filter iterations will over-smooth the surface
  fluidMapper->SetSurfaceFilterIterations(3);

  // Set the filter radius for smoothing the depth surface
  // This is an optional parameter, default value is 5
  fluidMapper->SetSurfaceFilterRadius(5);

  // Set the filtering method, it's up to personal choice
  // This is an optional parameter, default value is NarrowRange, other value is
  // BilateralGaussian
  fluidMapper->SetSurfaceFilterMethod(vtkOpenGLFluidMapper::FluidSurfaceFilterMethod::NarrowRange);

  // Set the display method, from transparent volume to opaque surface etc
  // Default value is TransparentFluidVolume
  fluidMapper->SetDisplayMode(vtkOpenGLFluidMapper::FluidDisplayMode::TransparentFluidVolume);

#ifdef BLUE_WATER
  // Set the volume attenuation color (color that will be absorbed
  // exponentially through the fluid volume) (below is the attenuation color
  // that will produce blue volume fluid)
  fluidMapper->SetAttenuationColor(0.8f, 0.2f, 0.15f);

  // Set the attenuation scale, which will be multiplied with the
  // attenuation color Default value is 1.0
  fluidMapper->SetAttenuationScale(1.0f);
#else // Not BLUE_WATER
  // This is blood
  fluidMapper->SetAttenuationColor(0.2f, 0.95f, 0.95f);
  fluidMapper->SetAttenuationScale(3.0f);
#endif

  // Set the surface color (applicable only if the display mode is
  // <Filter/Unfiltered>OpaqueSurface)
  fluidMapper->SetOpaqueColor(0.0f, 0.0f, 0.9f);

  // Set the particle color power and scale
  // (applicable only if there is color data for each point)
  // The particle color is then recomputed as newColor = pow(oldColor, power) *
  // scale
  fluidMapper->SetParticleColorPower(0.1f);
  fluidMapper->SetParticleColorScale(0.57f);

  // Set the additional reflection parameter, to add more light reflecting off
  // the surface Default value is 0.0
  fluidMapper->SetAdditionalReflection(0.0f);

  // Set the refractive index (1.33 for water)
  // Default value is 1.33
  fluidMapper->SetRefractiveIndex(1.33f);

  // Set the refraction scale, this will explicity change the amount of
  // refraction Default value is 1
  fluidMapper->SetRefractionScale(0.07f);

  // <========== end parameters turning for fluid mapper

  vtkNew<vtkVolume> vol;
  vol->SetMapper(fluidMapper);
  renderer->AddVolume(vol);
  //------------------------------------------------------------
  vtkNew<vtkTimerLog> timer;
  renderer->GetActiveCamera()->SetPosition(10, 2, 20);
  renderer->GetActiveCamera()->SetFocalPoint(1, 1, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->GetActiveCamera()->SetViewAngle(40.0);
  renderer->GetActiveCamera()->Dolly(1.7);
  renderer->ResetCameraClippingRange();
  timer->StartTimer();
  for (int i = 0; i < 3; ++i)
  {
    renderWindow->Render();
  }
  timer->StopTimer();
  cerr << "Render time: " << timer->GetElapsedTime() << endl;

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkInteractorStyleSwitch> style;
    style->SetCurrentStyleToTrackballCamera();
    iren->SetInteractorStyle(style);
    iren->Start();
  }
  //------------------------------------------------------------
  return !retVal;
}
