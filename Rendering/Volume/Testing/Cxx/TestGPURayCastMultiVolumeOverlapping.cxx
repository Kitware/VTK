/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMultiVolumeOverlapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Tests rendering 3 overlapping volumes as inputs in vtkGPUVolumeRCMapper
 * vtkMultiVolume.
 */
#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageResample.h"
#include "vtkImageResize.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMultiVolume.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVolume16Reader.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLImageDataReader.h"

#include "vtkAbstractMapper.h"
#include "vtkImageData.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"

#include "vtkMath.h"
#include <chrono>

namespace
{
class MoveRotateCommand : public vtkCommand
{
public:
  static MoveRotateCommand* New() { return new MoveRotateCommand; }

  void Execute(vtkObject* caller, unsigned long eventId, void* /*data*/) override
  {
    switch (eventId)
    {
      case vtkCommand::KeyPressEvent:
      {
        auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
        const std::string key = interactor->GetKeySym();

        double times[3] = { 0, 0, 0 };
        double timesAngle[3] = { 0, 0, 0 };
        // Translations
        if (key == "Left")
        {
          times[0] = -1;
        }
        else if (key == "Right")
        {
          times[0] = 1;
        }
        else if (key == "Down")
        {
          times[1] = -1;
        }
        else if (key == "Up")
        {
          times[1] = 1;
        }
        else if (key == "n")
        {
          times[2] = -1;
        }
        else if (key == "m")
        {
          times[2] = 1;
        }
        // Rotations
        else if (key == "a")
        {
          timesAngle[0] = -1;
        }
        else if (key == "s")
        {
          timesAngle[0] = 1;
        }
        else if (key == "z")
        {
          timesAngle[1] = -1;
        }
        else if (key == "x")
        {
          timesAngle[1] = 1;
        }
        else if (key == "c")
        {
          timesAngle[2] = -1;
        }
        else if (key == "v")
        {
          timesAngle[2] = 1;
        }
        else
          break;

        const double deltaX = times[0] * Delta;
        const double deltaY = times[1] * Delta;
        const double deltaZ = times[2] * Delta;
        const auto pos = this->Volume->GetPosition();
        this->Volume->SetPosition(pos[0] + deltaX, pos[1] + deltaY, pos[2] + deltaZ);

        const double rotX = timesAngle[0] * DeltaAngle;
        this->Volume->RotateX(rotX);
        const double rotY = timesAngle[1] * DeltaAngle;
        this->Volume->RotateY(rotY);
        const double rotZ = timesAngle[2] * DeltaAngle;
        this->Volume->RotateZ(rotZ);
      }
      break;
    }

    this->RenderWindow->Render();
  };

  void SetVolume(vtkProp3D* vol) { this->Volume = vol; }

  vtkRenderWindow* RenderWindow = nullptr;
  vtkProp3D* Volume = nullptr;
  double Delta = 10.0;
  double DeltaAngle = 5.0;
};
}

////////////////////////////////////////////////////////////////////////////////
int TestGPURayCastMultiVolumeOverlapping(int argc, char* argv[])
{
  // Load data
  vtkNew<vtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetImageRange(1, 93);
  reader->SetDataByteOrderToLittleEndian();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;
  reader->SetDataSpacing(3.2, 3.2, 1.5);

  vtkNew<vtkXMLImageDataReader> vaseSource;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  vaseSource->SetFileName(volumeFile);
  delete[] volumeFile;

  vtkSmartPointer<vtkXMLImageDataReader> xmlReader = vtkSmartPointer<vtkXMLImageDataReader>::New();
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/hncma-atlas.vti");
  xmlReader->SetFileName(filename);
  xmlReader->Update();
  delete[] filename;
  filename = nullptr;

  // Geometry
  vtkNew<vtkConeSource> coneSource;
  coneSource->SetRadius(30.);
  coneSource->SetHeight(50.);
  coneSource->SetResolution(40);
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  vtkNew<vtkActor> cone;
  cone->SetMapper(coneMapper);
  cone->RotateX(35.);
  cone->RotateY(-245.);
  cone->SetPosition(160., 90., -30.);

  vtkNew<vtkAxesActor> axis;
  axis->SetTotalLength(100., 100., 100.);
  axis->SetNormalizedTipLength(0.1, 0.1, 0.1);
  axis->SetNormalizedShaftLength(1., 1., 1.);
  axis->AxisLabelsOff();
  axis->SetConeRadius(0.5);

  // Volume 0 (upsampled headmr)
  // ---------------------------
  vtkNew<vtkImageResize> headmrSource;
  headmrSource->SetInputConnection(reader->GetOutputPort());
  headmrSource->SetResizeMethodToOutputDimensions();
  headmrSource->SetOutputDimensions(128, 128, 128);
  headmrSource->Update();

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.15);
  pf->AddPoint(1000, 0.15);
  pf->AddPoint(1150, 0.85);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.1);
  gf->AddPoint(100, 0.7);

  vtkNew<vtkVolume> vol;
  vol->GetProperty()->SetScalarOpacity(pf);
  vol->GetProperty()->SetColor(ctf);
  vol->GetProperty()->SetGradientOpacity(gf);
  vol->GetProperty()->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  // Note: Shading is currently not supported with multi-volume active
  //->ShadeOn();

  // Volume 1 (vase)
  // -----------------------------
  vtkNew<vtkColorTransferFunction> ctf1;
  ctf1->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf1->AddRGBPoint(500, 0.1, 1.0, 0.3);
  ctf1->AddRGBPoint(1000, 0.1, 1.0, 0.3);
  ctf1->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  vtkNew<vtkPiecewiseFunction> pf1;
  pf1->AddPoint(0, 0.0);
  pf1->AddPoint(500, 1.0);

  vtkNew<vtkPiecewiseFunction> gf1;
  gf1->AddPoint(0, 0.0);
  gf1->AddPoint(550, 1.0);

  vtkNew<vtkVolume> vol1;
  vol1->GetProperty()->SetScalarOpacity(pf1);
  vol1->GetProperty()->SetColor(ctf1);
  vol1->GetProperty()->SetGradientOpacity(gf1);
  vol1->GetProperty()->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vol1->RotateX(-55.);
  vol1->SetPosition(80., 50., 130.);

  // Volume 2 (brain)
  // -----------------------------
  vtkNew<vtkPiecewiseFunction> pf2;
  pf1->AddPoint(0, 0.0);
  pf1->AddPoint(5022, 0.09);

  vtkNew<vtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0, 1.0, 0.3, 0.2);
  ctf2->AddRGBPoint(2511, 0.3, 0.2, 0.9);
  ctf2->AddRGBPoint(5022, 0.5, 0.6, 1.0);

  vtkNew<vtkPiecewiseFunction> gf2;
  gf2->AddPoint(0, 0.0);
  gf2->AddPoint(550, 0.5);

  vtkNew<vtkVolume> vol2;
  vol2->GetProperty()->SetScalarOpacity(pf2);
  vol2->GetProperty()->SetColor(ctf2);
  // vol2->GetProperty()->SetGradientOpacity(gf2);
  vol2->GetProperty()->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vol2->SetScale(0.8, 0.8, 0.8);
  vol2->SetPosition(210., 200., -90.);
  vol2->RotateX(90.);
  vol2->RotateY(-95.);
  vol2->RotateZ(-5.);

  // Multi volume instance
  // ---------------------
  // Create an overlapping volume prop (add specific properties to each
  // entity).
  vtkNew<vtkMultiVolume> overlappingVol;
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->UseJitteringOn();
  overlappingVol->SetMapper(mapper);

  mapper->SetInputConnection(0, headmrSource->GetOutputPort());
  overlappingVol->SetVolume(vol, 0);

  mapper->SetInputConnection(2, vaseSource->GetOutputPort());
  overlappingVol->SetVolume(vol1, 2);

  mapper->SetInputConnection(4, xmlReader->GetOutputPort());
  overlappingVol->SetVolume(vol2, 4);

  // Rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(1.0, 1.0, 1.0);

  ren->AddActor(axis);
  ren->AddActor(cone);
  ren->AddVolume(overlappingVol);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<MoveRotateCommand> moveComm;
  moveComm->SetVolume(vol1);
  moveComm->RenderWindow = renWin;
  iren->AddObserver(vtkCommand::KeyPressEvent, moveComm);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  auto cam = ren->GetActiveCamera();
  cam->SetFocalPoint(41.9596, -17.9662, 78.5903);
  cam->SetPosition(373.891, 619.954, -53.5932);
  cam->SetViewUp(-0.0358384, -0.184856, -0.982112);
  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
