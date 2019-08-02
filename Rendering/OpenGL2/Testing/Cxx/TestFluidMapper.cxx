/*=========================================================================

 Program:   Visualization Toolkit
 Module:    TestSprites.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSkybox.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkLight.h"
#include "vtkLightActor.h"
#include "vtkProperty.h"
#include "vtkCallbackCommand.h"
#include "vtkImageData.h"
#include "vtkTestUtilities.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkRegressionTestImage.h"

#include "vtkOpenGLFluidMapper.h"

#include <queue>
#include <array>

//-----------------------------------------------------------------------------
// Disable this to do unit test
// Enable this for interactive demonstration
#define INTERACTIVE_DEMO

// Define this to render blue color water, otherwise will render red (blood) color
// And if define VERTEX_COLOR below, colorful fluid will be rendered
#define BLUE_WATER

// Define this to add particle color to particles
//#define VERTEX_COLOR

// Define this to animation the dragon
#define ANIMATE_DRAGON

// Display light actor
#define DISPLAY_LIGHT_ACTOR

// Fluid mapper, need to be global static to control parameters interactively
static vtkNew<vtkOpenGLFluidMapper> g_FluidMapper;

// Global variables for particle data
static vtkNew<vtkActor>  g_Dragon;
static vtkNew<vtkPoints> g_Points;
static vtkNew<vtkPoints> g_Colors;
constexpr static double  g_DragonPos[3] { 2, -0.5, 3 };
//constexpr static double g_DragonPos[3] { 2, -0.5, 0 };

constexpr static float g_ParticleRadius = 0.03f;
constexpr static float g_Spacing        = 2.0f * g_ParticleRadius;

constexpr float colorRamp[] = { 1.0, 0.0, 0.0,
                                1.0, 0.5, 0.0,
                                1.0, 1.0, 0.0,
                                1.0, 0.0, 1.0,
                                0.0, 1.0, 0.0,
                                0.0, 1.0, 1.0,
                                0.0, 0.5, 1.0 };

std::array<float, 3> getColorRamp(float x) {
    while(x > 1.0f) {
        x -= 1.0f;
    }
    const float segmentSize = 1.0f / 6.0f;
    float       segment     = floor(x / segmentSize);
    float       t           = (x - segmentSize * segment) / segmentSize;

    return std::array<float, 3> {
        (1.0f - t) * colorRamp[int(segment) * 3] + t * colorRamp[int(segment) + 1],
        (1.0f - t) * colorRamp[int(segment) * 3 + 1] + t * colorRamp[(int(segment) + 1) * 3 + 1],
        (1.0f - t) * colorRamp[int(segment) * 3 + 2] + t * colorRamp[(int(segment) + 1) * 3 + 2]
    };
}

//-----------------------------------------------------------------------------
#ifdef INTERACTIVE_DEMO
// Global variables for animation pause/resume
static bool g_Animation = true;

// Random number from [-1, 1]
float rand11() { return 2.0f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f; }

// Pause/resume animation by pressing spacebar
// Press 'd' to change display mode
// Press 'm' to change filter method
void keypressFunc(vtkObject* caller, unsigned long, void*, void*) {
    const auto iren = static_cast<vtkRenderWindowInteractor*>(caller);
    if(iren->GetKeyCode() == ' ') {
        g_Animation = !g_Animation;
    } else if(iren->GetKeyCode() == 'd') {
        auto mode = static_cast<int>(g_FluidMapper->GetDisplayMode());
        mode = (mode + 1) % vtkOpenGLFluidMapper::NumDisplayModes;
        g_FluidMapper->SetDisplayMode(static_cast<vtkOpenGLFluidMapper::FluidDisplayMode>(mode));
        static_cast<vtkRenderWindowInteractor*>(caller)->Render();
    } else if(iren->GetKeyCode() == 'm') {
        auto filter = static_cast<int>(g_FluidMapper->GetSurfaceFilterMethod());
        filter = (filter + 1) % vtkOpenGLFluidMapper::NumFilterMethods;
        g_FluidMapper->SetSurfaceFilterMethod(static_cast<vtkOpenGLFluidMapper::FluidSurfaceFilterMethod>(filter));
        static_cast<vtkRenderWindowInteractor*>(caller)->Render();
    }
}

// Update particle animation data
void updateFunc(vtkObject* caller, unsigned long, void*, void*) {
    if(!g_Animation) {
        return;
    }

    g_Points->Reset();
    g_Colors->Reset();

    // Max number of particle layers in x dimension
    constexpr static uint32_t maxLayers = static_cast<uint32_t>(17.0f / g_Spacing);

    // Each time step, move particles by (spacing * stepRatio) distance
    constexpr static float stepRatio = 0.5f;

    // Start position of the particles in the x dimension
    constexpr static float startX = -10.0f;

    // Min height and height variation of the fluid wave
    constexpr static int minHeight       = static_cast<uint32_t>(0.8f / g_Spacing);
    constexpr static int heightVariation = static_cast<uint32_t>(0.65f / g_Spacing);
    constexpr static int minZ = -static_cast<int>(1.0f / g_Spacing);
    constexpr static int maxZ = static_cast<int>(6.0f / g_Spacing);

    // Speed of the fluid wave
    constexpr static float waveSpeed = 5.0f;

    // Time step size
    constexpr static float timeStep = 0.006f;

    static std::queue<uint32_t>             layerSizeQueue;
    static std::deque<std::array<float, 3>> posQueue;
    static std::deque<std::array<float, 3>> colorQueue;
    static uint32_t                         layers = 0;
    static float                            t      = 0;
    static float                            lastX  = startX;

    // Remove the last fluid layer in the x dimension
    if(layers > maxLayers) {
        const auto layerSize = layerSizeQueue.front();
        layerSizeQueue.pop();
        for(uint32_t i = 0; i < layerSize; ++i) {
            posQueue.pop_front();
        }
#ifdef VERTEX_COLOR
        for(uint32_t i = 0; i < layerSize; ++i) {
            colorQueue.pop_front();
        }
#endif
        --layers;
    }

    // Shift particles to the right (positve x)
    for(auto& pos : posQueue) {
        pos[0] += g_Spacing * stepRatio;
        g_Points->InsertNextPoint(&pos[0]);
    }
#ifdef VERTEX_COLOR
    for(auto& color: colorQueue) {
        g_Colors->InsertNextPoint(&color[0]);
    }
#endif
    lastX += g_Spacing * stepRatio;
#ifdef ANIMATE_DRAGON
    g_Dragon->SetPosition(g_DragonPos[0],
                          g_DragonPos[1] + static_cast<double>(std::cos(waveSpeed * t)) * 0.5,
                          g_DragonPos[2]);
#endif

    // Append one more layer
    if(lastX >= startX + g_Spacing) {
        uint32_t layerSize = 0;
        int      height    = static_cast<int>(heightVariation * std::cos(waveSpeed * t) +
                                              heightVariation) + minHeight;
        for(int y = 0; y < height; ++y) {
            for(int z = minZ; z < maxZ; ++z) {
                ++layerSize;
                std::array<float, 3> pos { static_cast<float>(startX + 0.5f * rand11() * g_Spacing),
                                           static_cast<float>((y + 0.5f * rand11()) * g_Spacing),
                                           static_cast<float>((z + 0.5f * rand11()) * g_Spacing) };
                g_Points->InsertNextPoint(&pos[0]);
                posQueue.push_back(std::move(pos));
#ifdef VERTEX_COLOR
                const auto color = getColorRamp(t);
                g_Colors->InsertNextPoint(&color[0]);
                colorQueue.push_back(std::move(color));
#endif
            }
        }
        layerSizeQueue.push(layerSize);
        ++layers;
        lastX = startX;
    }

    t += timeStep;
    g_Points->Modified();
    static_cast<vtkRenderWindowInteractor*>(caller)->Render();
    //    std::cout << "Particle: " << g_Points->GetNumberOfPoints() << "\n";
    //    std::cout << "layers: " << layers << "\n";
    //    std::cout << "maxlayers: " << maxLayers << "\n";
}

#endif // #define INTERACTIVE_DEMO

//-----------------------------------------------------------------------------
int
TestFluidMapper(int argc, char* argv[]) {
    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.0, 0.0, 0.0);

    vtkNew<vtkRenderWindow> renderWindow;
#ifdef INTERACTIVE_DEMO
    renderWindow->SetSize(1920, 1080);
#else
    renderWindow->SetSize(400,   400);
#endif
    renderWindow->SetMultiSamples(0);
    renderWindow->AddRenderer(renderer);
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renderWindow);
    //------------------------------------------------------------
    // Create a light
    double lightPosition[3]   = { -10, 10, 0 };
    double lightFocalPoint[3] = { 0, 0, 0 };

    vtkNew<vtkLight> light;
    light->SetLightTypeToSceneLight();
    light->SetPosition(lightPosition[0], lightPosition[1], lightPosition[2]);
    light->SetPositional(true); // required for vtkLightActor below
    light->SetConeAngle(60);
    light->SetFocalPoint(lightFocalPoint[0], lightFocalPoint[1], lightFocalPoint[2]);
    light->SetDiffuseColor(1, 1, 1);
    light->SetAmbientColor(0.1, 0.1, 0.1);
    light->SetSpecularColor(1, 1, 1);
    //  renderer->AddLight(light); // can't do this here - must do this after the renderWindow->Render() below

    // Display where the light is
#ifdef DISPLAY_LIGHT_ACTOR
    vtkNew<vtkLightActor> lightActor;
    lightActor->SetLight(light);
    renderer->AddViewProp(lightActor);
#endif
    //------------------------------------------------------------
    const char*          fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
    vtkNew<vtkPLYReader> reader;
    reader->SetFileName(fileName);
    reader->Update();
    delete[] fileName;

    vtkNew<vtkPolyDataMapper> dragonMapper;
    dragonMapper->SetInputConnection(reader->GetOutputPort());
    g_Dragon->SetMapper(dragonMapper);
    g_Dragon->SetScale(20, 20, 20);
    g_Dragon->SetPosition(g_DragonPos[0], g_DragonPos[1], g_DragonPos[2]);
    g_Dragon->GetProperty()->SetColor(0.780392, 0.568627, 0.113725);
    renderer->AddActor(g_Dragon);
    //------------------------------------------------------------
    fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wintersun.jpg");
    vtkNew<vtkJPEGReader> imgReaderSkyBox;
    imgReaderSkyBox->SetFileName(fileName);
    vtkNew<vtkTexture> skbTexture;
    skbTexture->InterpolateOn();
    skbTexture->SetInputConnection(imgReaderSkyBox->GetOutputPort());
    delete[] fileName;

    vtkNew<vtkSkybox> skybox;
    skybox->SetProjectionToSphere();
    skybox->SetTexture(skbTexture);
    skybox->ForceOpaqueOn();
    renderer->AddActor(skybox);
    //------------------------------------------------------------
    vtkNew<vtkTexture> floorTexture;
    floorTexture->InterpolateOn();
    floorTexture->RepeatOn();

    // Enable this if we have a floor texture
#if 0
    // Load a floor texture from disk
    fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/marble.jpg");
    vtkNew<vtkJPEGReader> imgReaderFloorTex;
    imgReaderFloorTex->SetFileName(fileName);
    floorTexture->SetInputConnection(imgReaderFloorTex->GetOutputPort());
    delete[] fileName;
#else
    // If there is not any floor texture existed yet, let try to create a texture on the fly
    vtkNew<vtkImageData> imageData;
    imageData->SetDimensions(2, 2, 1);
    imageData->AllocateScalars(VTK_INT, 1);

    constexpr static int whiteColor = 200;
    constexpr static int grayColor  = 100;

    int* data = static_cast<int*>(imageData->GetScalarPointer(0, 0, 0));
    data[0] = grayColor;

    data    = static_cast<int*>(imageData->GetScalarPointer(0, 1, 0));
    data[0] = whiteColor;

    data    = static_cast<int*>(imageData->GetScalarPointer(1, 0, 0));
    data[0] = whiteColor;

    data    = static_cast<int*>(imageData->GetScalarPointer(1, 1, 0));
    data[0] = grayColor;

    vtkNew<vtkLookupTable> table;
    table->SetTableRange(0, 255);
    table->SetValueRange(0.0, 1.0);
    table->SetSaturationRange(0.0, 0.0);
    table->SetHueRange(0.0, 0.0);
    table->SetAlphaRange(1.0, 1.0);
    table->SetNumberOfColors(256);
    table->Build();

    floorTexture->SetLookupTable(table);
    floorTexture->SetInputData(imageData);
#endif

    vtkNew<vtkPlaneSource> plane;
    plane->SetNormal(0.0, -1.0, 0.0);
    plane->SetCenter(-20.0, 0.0, -20.0);
    plane->SetPoint1(20, 0, -20);
    plane->SetPoint2(-20, 0, 20);
    plane->Update();

    const auto            planeData = plane->GetOutput();
    vtkNew<vtkFloatArray> textureCoordinates;
    textureCoordinates->SetNumberOfComponents(2);
    textureCoordinates->SetName("TextureCoordinates");

    // Scale the floor texture 5 times larger
    constexpr static float texScale = 10.0;
    float                  tuple[2] = { 0.0, 0.0 };
    textureCoordinates->InsertNextTuple(tuple);
    tuple[0] = texScale; tuple[1] = 0.0;
    textureCoordinates->InsertNextTuple(tuple);
    tuple[0] = texScale; tuple[1] = texScale;
    textureCoordinates->InsertNextTuple(tuple);
    tuple[0] = 0.0; tuple[1] = texScale;
    textureCoordinates->InsertNextTuple(tuple);
    planeData->GetPointData()->SetTCoords(textureCoordinates);

    vtkNew<vtkPolyDataMapper> planeMapper;
    planeMapper->SetInputData(planeData);
    vtkNew<vtkActor> texturedPlane;
    texturedPlane->SetMapper(planeMapper);
    texturedPlane->SetTexture(floorTexture);
    renderer->AddActor(texturedPlane);
    //------------------------------------------------------------

#ifndef INTERACTIVE_DEMO
    const float spacing = 0.1f;
    for(int z = 0; z < 50; ++z) {
        for(int y = 0; y < 15; ++y) {
            for(int x = 0; x < 50; ++x) {
                g_Points->InsertNextPoint(static_cast<double>(x * spacing),
                                          static_cast<double>(y * spacing),
                                          static_cast<double>(z * spacing));
            }
        }
    }
#else
    vtkNew<vtkCallbackCommand> updateCallback;
    vtkNew<vtkCallbackCommand> keypressCallback;
    updateCallback->SetCallback(updateFunc);
    keypressCallback->SetCallback(keypressFunc);

    iren->AddObserver(vtkCommand::TimerEvent,      updateCallback);
    iren->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);
    iren->Initialize();
    iren->CreateRepeatingTimer(0);
#endif

    vtkNew<vtkPolyData> pointData;
    pointData->SetPoints(g_Points);
#ifdef VERTEX_COLOR
    vtkNew<vtkPolyData> colorData;
    colorData->SetPoints(g_Colors);
    g_FluidMapper->SetInputData(pointData, colorData);
#else
    g_FluidMapper->SetInputData(pointData);
#endif

    // Begin parameters turning for fluid mapper ==========>
    // For new dataset, we may need to do multiple times turning parameters until we get a nice result
    // Must set parameter is particle radius,

    // MUST SET PARAMETER ==========================
    // Set the radius of the rendered spheres to be 2 times larger than the actual sphere radius
    // This is necessary to fuse the gaps between particles and obtain a smooth surface
    g_FluidMapper->SetParticleRadius(g_ParticleRadius * 3.0f);

    // Set the number of iterations to filter the depth surface
    // This is an optional parameter, default value is 3
    // Usually set this to around 3-5
    // Too many filter iterations will over-smooth the surface
    g_FluidMapper->SetSurfaceFilterIterations(3);

    // Set the filter radius for smoothing the depth surface
    // This is an optional parameter, default value is 5
    g_FluidMapper->SetSurfaceFilterRadius(5);

    // Set the filtering method, it's up to personal choice
    // This is an optional parameter, default value is NarrowRange, other value is BilateralGaussian
    g_FluidMapper->SetSurfaceFilterMethod(vtkOpenGLFluidMapper::FluidSurfaceFilterMethod::NarrowRange);

    // Set the display method, from transparent volume to opaque surface etc
    // Default value is TransparentFluidVolume
    g_FluidMapper->SetDisplayMode(vtkOpenGLFluidMapper::FluidDisplayMode::TransparentFluidVolume);

#ifdef BLUE_WATER
    // Set the volume attennuation color (color that will be absorpted exponentially through the fluid volume)
    // (below is the attennuation color that will produce blue volume fluid)
    g_FluidMapper->SetAttennuationColor(0.8f, 0.2f, 0.15f);

    // Set the attennuation scale, which will be multiple with attennuation color
    // Default value is 1.0
#if defined(VERTEX_COLOR) || !defined(INTERACTIVE_DEMO)
    g_FluidMapper->SetAttennuationScale(1.0f);
#else
    g_FluidMapper->SetAttennuationScale(0.5f);
#endif
#else // Not BLUE_WATER
    // This is blood
    g_FluidMapper->SetAttennuationColor(0.2f, 0.95f, 0.95f);
    g_FluidMapper->SetAttennuationScale(3.0f);
#endif

    // Set the surface color (applicable only if the display mode is <Filter/Unfiltered>OpaqueSurface)
    g_FluidMapper->SetOpaqueColor(0.0f, 0.0f, 0.9f);

    // Set the particle color power and scale
    // (applicable only if there is color data for each point)
    // The particle color is then recomputed as newColor = pow(oldColor, power) * scale
#ifdef VERTEX_COLOR
    g_FluidMapper->SetParticleColorPower(0.1f);
    g_FluidMapper->SetParticleColorScale(0.57f);
#endif

    // Set the additional reflection parameter, to add more light reflecting off the surface
    // Default value is 0.0
    g_FluidMapper->SetAdditionalReflection(0.0f);

    // Set the refractive index (1.33 for water)
    // Default value is 1.33
    g_FluidMapper->SetRefractiveIndex(1.33f);

    // Set the refraction scale, this will explicity change the amount of refraction
    // Default value is 1
#ifdef INTERACTIVE_DEMO
    g_FluidMapper->SetRefractionScale(0.01f);
#else
    g_FluidMapper->SetRefractionScale(1.f);
#endif

    // <========== end parameters turning for fluid mapper

    vtkNew<vtkVolume> vol;
    vol->SetMapper(g_FluidMapper);
    renderer->AddVolume(vol);
    //------------------------------------------------------------
    vtkNew<vtkTimerLog> timer;
#ifdef INTERACTIVE_DEMO
    renderer->GetActiveCamera()->SetPosition(-10, 30, 40);
#else
    renderer->GetActiveCamera()->SetPosition(2, 15, 20);
#endif
    renderer->GetActiveCamera()->SetFocalPoint(2, 0, 0);
    renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
    renderer->GetActiveCamera()->SetViewAngle(70.0);
    renderer->GetActiveCamera()->Dolly(3.0);
    renderer->ResetCameraClippingRange();
    timer->StartTimer();
    renderWindow->Render();
    renderer->AddLight(light); // must do this after renderWindow->Render();
    timer->StopTimer();
    cerr << "Render time: " << timer->GetElapsedTime() << endl;

    int retVal = vtkRegressionTestImage(renderWindow);
    if(retVal == vtkRegressionTester::DO_INTERACTOR) {
        vtkNew<vtkInteractorStyleSwitch> style;
        style->SetCurrentStyleToTrackballCamera();
        iren->SetInteractorStyle(style);
        iren->Start();
    }
    //------------------------------------------------------------
    return !retVal;
}
