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
    renWindow->SetSize(500, 500);
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
    renWindow->SetSize(600, 600);
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
    renWindow->SetSize(600,600);
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
  volumeTest(const char *name) : vtkRTTest(name)
    {
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
    wavelet->SetWholeExtent(0, 100*res1 -1,
                            0, 100*res2 -1,
                            0, 100*res3 -1);
    wavelet->Update();

    vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
    volumeMapper->SetInputConnection(wavelet->GetOutputPort());

    vtkNew<vtkVolumeProperty> volumeProperty;
    vtkNew<vtkColorTransferFunction> ctf;
    ctf->AddHSVPoint(37.3531, 0.3, 1.0, 1);
    ctf->AddHSVPoint(100.091, 0.0, 1.0, 1.0);
    ctf->AddHSVPoint(276.829, 0.0, 0.2, 1.0);
    ctf->SetColorSpaceToHSV();

    vtkNew<vtkPiecewiseFunction> pwf;
    pwf->AddPoint(37.3531, 0.0);
    pwf->AddPoint(276.829, 1.0/res1);

    volumeProperty->SetColor(ctf.GetPointer());
    volumeProperty->SetScalarOpacity(pwf.GetPointer());

    vtkNew<vtkVolume> volume;
    volume->SetMapper(volumeMapper.GetPointer());
    volume->SetProperty(volumeProperty.GetPointer());

    // create a rendering window and renderer
    vtkNew<vtkRenderer> ren1;
    vtkNew<vtkRenderWindow> renWindow;
    renWindow->AddRenderer(ren1.GetPointer());
    ren1->AddActor(volume.GetPointer());

    // set the size/color of our window
    renWindow->SetSize(600, 600);
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
};

