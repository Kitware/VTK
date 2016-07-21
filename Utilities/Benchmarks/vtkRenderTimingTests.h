/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderTimingTests.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkRenderTimingTests_h
#define vtkRenderTimingTests_h

/*
To add a test you must define a subclass of vtkRTTest and implement the
pure virtual functions. Then in the main section at the bottom of this
file add your test to the tests to be run and rebuild. See some of the
existing tests to get an idea of what to do.
*/

#include "vtkRenderTimings.h"

#include "vtkAutoInit.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderingOpenGLConfigure.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkNew.h"

/*=========================================================================
Define a test for simple triangle mesh surfaces
=========================================================================*/
#include "vtkParametricBoy.h"
#include "vtkParametricTorus.h"
#include "vtkParametricFunctionSource.h"

class surfaceTest : public vtkRTTest
{
  public:
  surfaceTest(const char *name,
    bool withColors, bool withNormals) : vtkRTTest(name)
  {
    this->WithColors = withColors;
    this->WithNormals = withNormals;
  }

  const char *GetSummaryResultName() { return "Mtris/sec"; }

  const char *GetSecondSummaryResultName() { return "Mtris"; }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats,
      int /*argc*/, char * /* argv */[])
    {
    int ures, vres;
    ats->GetSequenceNumbers(ures,vres);

    // ------------------------------------------------------------
    // Create surface
    // ------------------------------------------------------------
    //  vtkNew<vtkParametricBoy> PB;
    vtkNew<vtkParametricTorus> PB;
    vtkNew<vtkParametricFunctionSource> PFS;
    PFS->SetParametricFunction(PB.Get());
    if (this->WithColors)
      {
      PFS->SetScalarModeToPhase();
      }
    else
      {
      PFS->SetScalarModeToNone();
      }
    if (this->WithNormals == false)
      {
      PFS->GenerateNormalsOff();
      }
    PFS->SetUResolution(ures * 50);
    PFS->SetVResolution(vres * 100);
    PFS->Update();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(PFS->GetOutputPort());
    mapper->SetScalarRange(0.0, 360.0);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.Get());
    ren1->AddActor(actor.Get());

    // set the size/color of our window
    renWindow->SetSize(this->GetRenderWidth(), this->GetRenderHeight());
    ren1->SetBackground(0.2, 0.3, 0.5);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;
    ren1->GetActiveCamera()->Azimuth(90);
    ren1->ResetCameraClippingRange();

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(1);
      ren1->GetActiveCamera()->Elevation(1);
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime) >
          this->TargetTime * 1.5)
        {
        frameCount = i + 1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime() - startTime -
                                  firstFrameTime) / frameCount;
    double numTris = PFS->GetOutput()->GetPolys()->GetNumberOfCells();

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["Mtris"] = 1.0e-6 * numTris;
    result.Results["Mtris/sec"] = 1.0e-6 * numTris / subsequentFrameTime;
    result.Results["triangles"] = numTris;

    return result;
    }

  protected:
  bool WithNormals;
  bool WithColors;
};

/*=========================================================================
Define a test for glyphing
=========================================================================*/
#include "vtkGlyph3DMapper.h"
#include "vtkPlaneSource.h"
#include "vtkElevationFilter.h"
#include "vtkSphereSource.h"

class glyphTest : public vtkRTTest
{
  public:
  glyphTest(const char *name) : vtkRTTest(name)
  {
  }

  const char *GetSummaryResultName() { return "Mtris/sec"; }

  const char *GetSecondSummaryResultName() { return "triangles"; }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats,
      int /*argc*/, char * /* argv */[])
    {
    int res1, res2, res3, res4;
    ats->GetSequenceNumbers(res1, res2, res3, res4);

    // create
    vtkNew<vtkPlaneSource> plane;
    plane->SetResolution(res1 * 10, res2 * 10);
    plane->SetOrigin(-res1 * 5.0, -res2 * 5.0, 0.0);
    plane->SetPoint1(res1 * 5.0,-res2 * 5.0, 0.0);
    plane->SetPoint2(-res1 * 5.0,res2 * 5.0, 0.0);
    vtkNew<vtkElevationFilter> colors;
    colors->SetInputConnection(plane->GetOutputPort());
    colors->SetLowPoint(plane->GetOrigin());
    colors->SetHighPoint(res1 * 5.0, res2 * 5.0, 0.0);

    // create simple poly data so we can apply glyph
    vtkNew<vtkSphereSource> sphere;
    sphere->SetPhiResolution(5 * res3 + 2);
    sphere->SetThetaResolution(10 * res4);
    sphere->SetRadius(0.7);

    vtkNew<vtkGlyph3DMapper> mapper;
    mapper->SetInputConnection(colors->GetOutputPort());
    mapper->SetSourceConnection(sphere->GetOutputPort());
    mapper->SetScalarRange(0.0, 2.0);

    // vtkNew<vtkPolyDataMapper> mapper;
    // mapper->SetInputConnection(colors->GetOutputPort());
    // mapper->SetScalarRange(0.0,2.0);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.Get());
    ren1->AddActor(actor.Get());

    // set the size/color of our window
    renWindow->SetSize(this->GetRenderWidth(), this->GetRenderHeight());
    ren1->SetBackground(0.2, 0.3, 0.5);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(0.5);
      ren1->GetActiveCamera()->Elevation(0.5);
      ren1->GetActiveCamera()->Zoom(1.01);
      ren1->ResetCameraClippingRange();
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime) >
          this->TargetTime * 1.5)
        {
        frameCount = i + 1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime() - startTime -
                                  firstFrameTime)/frameCount;
    double numTris = 100.0 * res1 * res2 *
                     sphere->GetOutput()->GetPolys()->GetNumberOfCells();

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["Mtris"] = 1.0e-6 * numTris;
    result.Results["Mtris/sec"] = 1.0e-6 * numTris/subsequentFrameTime;
    result.Results["triangles"] = numTris;

    return result;
    }

  protected:
};


#ifdef HAVE_CHEMISTRY
/*=========================================================================
Define a test for molecules
=========================================================================*/
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkBoxMuellerRandomSequence.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"

#ifdef VTK_OPENGL2
VTK_MODULE_INIT(vtkDomainsChemistryOpenGL2);
#endif

class moleculeTest : public vtkRTTest
{
  public:
  moleculeTest(const char *name, bool atomsOnly = false) : vtkRTTest(name)
  {
    this->AtomsOnly = atomsOnly;
  }

  const char *GetSummaryResultName() {
    return this->AtomsOnly ? "Atoms/sec" : "Atoms+Bonds/sec"; }

  const char *GetSecondSummaryResultName() {
    return this->AtomsOnly ? "Atoms" : "Atoms+Bonds"; }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats,
      int /*argc*/, char * /* argv */[])
    {
    int res1;
    ats->GetSequenceNumbers(res1);

    vtkNew<vtkBoxMuellerRandomSequence> rs;
    vtkNew<vtkMolecule> mol;
    vtkNew<vtkPointLocator> pl;

    // build a molecule
    float scale = 3.0*pow(static_cast<double>(res1),0.33);
    double pos[3];
    vtkNew<vtkPolyData> pointSet;
    vtkNew<vtkPoints> pts;
    pointSet->SetPoints(pts.GetPointer());
    double bounds[6];
    bounds[0] = 0.0; bounds[2] = 0.0; bounds[4] = 0.0;
    bounds[1] = scale; bounds[3] = scale; bounds[5] = scale;
    pl->SetDataSet(pointSet.GetPointer());
    pl->InitPointInsertion(pointSet->GetPoints(), bounds, 10*res1);
    for (int i = 0; i < res1*100; i++)
      {
      pos[0] = scale*rs->GetValue(); rs->Next();
      pos[1] = scale*rs->GetValue(); rs->Next();
      pos[2] = scale*rs->GetValue(); rs->Next();
      pl->InsertPoint(i,pos);
      int molType = i%9 > 5 ? i%9 : 1; // a lot of H, some N O CA
      mol->AppendAtom(molType, pos[0], pos[1], pos[2]);
      }

    // now add some bonds
    if (!this->AtomsOnly)
      {
      vtkNew<vtkIdList> ids;
      int bondCount = 0;
      while(bondCount < res1*60)
        {
        pos[0] = scale*rs->GetValue(); rs->Next();
        pos[1] = scale*rs->GetValue(); rs->Next();
        pos[2] = scale*rs->GetValue(); rs->Next();
        pl->FindClosestNPoints(2, pos, ids.GetPointer());
        // are the atoms close enough?
        if (vtkMath::Distance2BetweenPoints(
            mol->GetAtomPosition(ids->GetId(0)).GetData(),
            mol->GetAtomPosition(ids->GetId(1)).GetData()) < 4.0)
          {
          int bondType = bondCount%10 == 9 ? 3 : (bondCount%10)/7+1;
          mol->AppendBond(ids->GetId(0), ids->GetId(1), bondType);
          bondCount++;
          }
        }
      }

    vtkNew<vtkMoleculeMapper> mapper;
    mapper->SetInputData(mol.GetPointer());
    mapper->UseBallAndStickSettings();

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.GetPointer());

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.GetPointer());
    ren1->AddActor(actor.GetPointer());

    // set the size/color of our window
    renWindow->SetSize(this->GetRenderWidth(), this->GetRenderHeight());
    ren1->SetBackground(0.2,0.3,0.5);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;
    ren1->GetActiveCamera()->Zoom(1.5);

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(0.5);
      ren1->GetActiveCamera()->Elevation(0.5);
      ren1->GetActiveCamera()->Zoom(1.01);
      //ren1->ResetCameraClippingRange();
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime)
            > this->TargetTime * 1.5)
        {
        frameCount = i+1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime()
      - startTime - firstFrameTime)/frameCount;
    double numAtoms = mol->GetNumberOfAtoms();

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["Atoms"] = numAtoms;
    result.Results["Bonds"] = mol->GetNumberOfBonds();
    result.Results["Atoms+Bonds"] = (numAtoms+mol->GetNumberOfBonds());
    result.Results["Atoms+Bonds/sec"] = (numAtoms+mol->GetNumberOfBonds())
                                        /subsequentFrameTime;
    result.Results["Atoms/sec"] = numAtoms/subsequentFrameTime;

    return result;
    }

  protected:
    bool AtomsOnly;
};
#endif

/*=========================================================================
Define a test for volume rendering
=========================================================================*/
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"
#include "vtkVolumeProperty.h"

class volumeTest : public vtkRTTest
{
  public:
  volumeTest(const char *name, bool withShading) : vtkRTTest(name)
    {
    this->WithShading = withShading;
    }

  const char *GetSummaryResultName()
    {
    return "Mvoxels/sec" ;
    }

  const char *GetSecondSummaryResultName()
    {
    return "Mvoxels";
    }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats,
      int /*argc*/, char * /* argv */[])
    {
    int res1, res2, res3;
    ats->GetSequenceNumbers(res1,res2,res3);

    vtkNew<vtkRTAnalyticSource> wavelet;
    wavelet->SetWholeExtent(-50*res1 - 1, 50*res1,
                            -50*res2 - 1, 50*res2,
                            -50*res3 - 1, 50*res3);
    wavelet->Update();

    vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
    volumeMapper->SetInputConnection(wavelet->GetOutputPort());
    volumeMapper->AutoAdjustSampleDistancesOff();
    volumeMapper->SetSampleDistance(0.9);

    vtkNew<vtkVolumeProperty> volumeProperty;
    vtkNew<vtkColorTransferFunction> ctf;
    ctf->AddRGBPoint(33.34, 0.23, 0.3, 0.75);
    ctf->AddRGBPoint(72.27, 0.79, 0.05, 0.22);
    ctf->AddRGBPoint(110.3, 0.8, 0.75, 0.82);
    ctf->AddRGBPoint(134.19, 0.78, 0.84, 0.04);
    ctf->AddRGBPoint(159.84, 0.07, 0.87, 0.43);
    ctf->AddRGBPoint(181.96, 0.84, 0.31, 0.48);
    ctf->AddRGBPoint(213.803, 0.73, 0.62, 0.8);
    ctf->AddRGBPoint(255.38, 0.75, 0.19, 0.05);
    ctf->AddRGBPoint(286.33, 0.7, 0.02, 0.15);
    ctf->SetColorSpaceToHSV();

    vtkNew<vtkPiecewiseFunction> pwf;
    pwf->AddPoint(33.35, 0.0);
    pwf->AddPoint(81.99, 0.01);
    pwf->AddPoint(128.88, 0.02);
    pwf->AddPoint(180.19, 0.03);
    pwf->AddPoint(209.38, 0.04);
    pwf->AddPoint(286.33, 0.05);

    volumeProperty->SetColor(ctf.GetPointer());
    volumeProperty->SetScalarOpacity(pwf.GetPointer());

    vtkNew<vtkVolume> volume;
    volume->SetMapper(volumeMapper.GetPointer());
    volume->SetProperty(volumeProperty.GetPointer());
    if (this->WithShading)
      {
      volumeProperty->ShadeOn();
      }

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.GetPointer());
    ren1->AddActor(volume.GetPointer());

    // set the size/color of our window
    renWindow->SetSize(this->GetRenderWidth(), this->GetRenderHeight());
    ren1->SetBackground(0.2, 0.3, 0.4);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;
    ren1->GetActiveCamera()->Zoom(1.2);
    ren1->ResetCameraClippingRange();

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(0.5);
      ren1->GetActiveCamera()->Elevation(0.5);
      ren1->ResetCameraClippingRange();
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime)
            > this->TargetTime * 1.5)
        {
        frameCount = i+1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime()
      - startTime - firstFrameTime)/frameCount;

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["Mvoxels/sec"] = static_cast<double>(res1*res2*res3)/subsequentFrameTime;
    result.Results["Mvoxels"] = res1 * res2 * res3;

    return result;
    }

  protected:
  bool WithShading;
};

/*=========================================================================
Define a test for depth peeling transluscent geometry.
=========================================================================*/
#include "vtkParametricTorus.h"
#include "vtkParametricFunctionSource.h"
#include "vtkProperty.h"
#include "vtkTransform.h"

class depthPeelingTest : public vtkRTTest
{
  public:
  depthPeelingTest(const char *name, bool withNormals)
    : vtkRTTest(name),
      WithNormals(withNormals)
  {
  }

  const char *GetSummaryResultName() { return "subsequent frame time"; }

  const char *GetSecondSummaryResultName() { return "first frame time"; }

  virtual vtkRTTestResult Run(vtkRTTestSequence *ats,
      int /*argc*/, char * /* argv */[])
    {
    int ures, vres;
    ats->GetSequenceNumbers(ures,vres);

    // ------------------------------------------------------------
    // Create surface
    // ------------------------------------------------------------
    vtkNew<vtkParametricTorus> PB;
    vtkNew<vtkParametricFunctionSource> PFS;
    PFS->SetParametricFunction(PB.Get());
    if (this->WithNormals == false)
      {
      PFS->GenerateNormalsOff();
      }
    PFS->SetUResolution(ures * 50);
    PFS->SetVResolution(vres * 100);
    PFS->Update();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(PFS->GetOutputPort());
    mapper->SetScalarRange(0.0, 360.0);

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->SetMultiSamples(0);
    renWindow->SetAlphaBitPlanes(1);
    renWindow->AddRenderer(ren1.Get());

    // Setup depth peeling to render an exact scene:
    ren1->UseDepthPeelingOn();
    ren1->SetMaximumNumberOfPeels(100);
    ren1->SetOcclusionRatio(0.);

    // Create a set of 10 colored translucent actors at slight offsets:
    const int NUM_ACTORS = 10;
    const unsigned char colors[NUM_ACTORS][4] = { { 255,   0,   0, 32 },
                                                  {   0, 255,   0, 32 },
                                                  {   0,   0, 255, 32 },
                                                  { 128, 128,   0, 32 },
                                                  {   0, 128, 128, 32 },
                                                  { 128,   0, 128, 32 },
                                                  { 128,  64,  64, 32 },
                                                  {  64, 128,  64, 32 },
                                                  {  64,  64, 128, 32 },
                                                  {  64,  64,  64, 32 } };

    for (int i = 0; i < NUM_ACTORS; ++i)
      {
      vtkNew<vtkActor> actor;
      actor->SetMapper(mapper.Get());
      actor->GetProperty()->SetColor(colors[i][0] / 255.,
                                     colors[i][1] / 255.,
                                     colors[i][2] / 255.);
      actor->GetProperty()->SetOpacity(colors[i][3] / 255.);

      vtkNew<vtkTransform> xform;
      xform->Identity();
      xform->RotateX(i * (180. / static_cast<double>(NUM_ACTORS)));
      actor->SetUserTransform(xform.Get());

      ren1->AddActor(actor.Get());
      }

    // set the size/color of our window
    renWindow->SetSize(this->GetRenderWidth(), this->GetRenderHeight());
    ren1->SetBackground(0.2, 0.3, 0.5);

    // draw the resulting scene
    double startTime = vtkTimerLog::GetUniversalTime();
    renWindow->Render();
    double firstFrameTime = vtkTimerLog::GetUniversalTime() - startTime;
    ren1->GetActiveCamera()->Azimuth(90);
    ren1->ResetCameraClippingRange();

    int frameCount = 80;
    for (int i = 0; i < frameCount; i++)
      {
      renWindow->Render();
      ren1->GetActiveCamera()->Azimuth(1);
      ren1->GetActiveCamera()->Elevation(1);
      if ((vtkTimerLog::GetUniversalTime() - startTime - firstFrameTime) >
          this->TargetTime * 1.5)
        {
        frameCount = i + 1;
        break;
        }
      }
    double subsequentFrameTime = (vtkTimerLog::GetUniversalTime() - startTime -
                                  firstFrameTime) / frameCount;
    double numTris = PFS->GetOutput()->GetPolys()->GetNumberOfCells();
    numTris *= NUM_ACTORS;

    vtkRTTestResult result;
    result.Results["first frame time"] = firstFrameTime;
    result.Results["subsequent frame time"] = subsequentFrameTime;
    result.Results["FPS"] = 1. / subsequentFrameTime;
    result.Results["triangles"] = numTris;

    return result;
    }

  protected:
  bool WithNormals;
};

#endif
