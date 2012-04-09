/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDataTypesMIP.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test volume renders the same dataset using 8 different data types
// (unsigned char, signed char, unsigned short, signed short, unsigned int
// int, float, and double). It uses compositing and no shading. The extents of
// the dataset are (0, 114, 0, 100, 0, 74).


#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkTestUtilities.h"
#include "vtkXMLImageDataReader.h"
#include "vtkImageShiftScale.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolumeProperty.h"
#include "vtkCamera.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkRegressionTestImage.h"

int TestGPURayCastDataTypesMIP(int argc,
                               char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  char *cfname=
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");

  vtkXMLImageDataReader *reader=vtkXMLImageDataReader::New();
  reader->SetFileName(cfname);
  delete [] cfname;

  vtkImageShiftScale *shiftScale[4][2];
  vtkColorTransferFunction *color[4][2];
  vtkPiecewiseFunction *opacity[4][2];

// unsigned char
  shiftScale[0][0]=vtkImageShiftScale::New();
  shiftScale[0][0]->SetInputConnection(reader->GetOutputPort());

  color[0][0]=vtkColorTransferFunction::New();
  color[0][0]->AddRGBPoint(0,0,0,1);
  color[0][0]->AddRGBPoint(255,0,1,0);

  opacity[0][0]=vtkPiecewiseFunction::New();
  opacity[0][0]->AddPoint(0,0);
  opacity[0][0]->AddPoint(255,1);

// signed char
  shiftScale[0][1]=vtkImageShiftScale::New();
  shiftScale[0][1]->SetInputConnection(shiftScale[0][0]->GetOutputPort());
  shiftScale[0][1]->SetShift(-128);
  shiftScale[0][1]->SetOutputScalarType(15);

  color[0][1]=vtkColorTransferFunction::New();
  color[0][1]->AddRGBPoint(-128,0,0,1);
  color[0][1]->AddRGBPoint(127,0,1,0);

  opacity[0][1]=vtkPiecewiseFunction::New();
  opacity[0][1]->AddPoint(-128,0);
  opacity[0][1]->AddPoint(127,1);

// unsigned short
  shiftScale[1][0]=vtkImageShiftScale::New();
  shiftScale[1][0]->SetInputConnection(reader->GetOutputPort());
  shiftScale[1][0]->SetScale(256);
  shiftScale[1][0]->SetOutputScalarTypeToUnsignedShort();

  color[1][0]=vtkColorTransferFunction::New();
  color[1][0]->AddRGBPoint(0,0,0,1);
  color[1][0]->AddRGBPoint(65535,0,1,0);

  opacity[1][0]=vtkPiecewiseFunction::New();
  opacity[1][0]->AddPoint(0,0);
  opacity[1][0]->AddPoint(65535,1);

//  short
  shiftScale[1][1]=vtkImageShiftScale::New();
  shiftScale[1][1]->SetInputConnection(shiftScale[1][0]->GetOutputPort());
  shiftScale[1][1]->SetShift(-32768);
  shiftScale[1][1]->SetOutputScalarTypeToShort();

  color[1][1]=vtkColorTransferFunction::New();
  color[1][1]->AddRGBPoint(-32768,0,0,1);
  color[1][1]->AddRGBPoint(32767,0,1,0);

  opacity[1][1]=vtkPiecewiseFunction::New();
  opacity[1][1]->AddPoint(-32768,0);
  opacity[1][1]->AddPoint(32767,1);

// unsigned int
  shiftScale[2][0]=vtkImageShiftScale::New();
  shiftScale[2][0]->SetInputConnection (reader->GetOutputPort());
  shiftScale[2][0]->SetScale(16777216);
  shiftScale[2][0]->SetOutputScalarTypeToUnsignedInt();

  color[2][0]=vtkColorTransferFunction::New();
  color[2][0]->AddRGBPoint(0,0,0,1);
  color[2][0]->AddRGBPoint(VTK_UNSIGNED_INT_MAX,0,1,0);

  opacity[2][0]=vtkPiecewiseFunction::New();
  opacity[2][0]->AddPoint(0,0);
  opacity[2][0]->AddPoint(VTK_UNSIGNED_INT_MAX,1);

// int
  shiftScale[2][1]=vtkImageShiftScale::New();
  shiftScale[2][1]->SetInputConnection(shiftScale[2][0]->GetOutputPort());
  shiftScale[2][1]->SetShift(VTK_INT_MIN);
  shiftScale[2][1]->SetOutputScalarTypeToInt();

  color[2][1]=vtkColorTransferFunction::New();
  color[2][1]->AddRGBPoint(VTK_INT_MIN,0,0,1);
  color[2][1]->AddRGBPoint(VTK_INT_MAX,0,1,0);

  opacity[2][1]=vtkPiecewiseFunction::New();

  opacity[2][1]->AddPoint(VTK_INT_MIN,0);
  opacity[2][1]->AddPoint(VTK_INT_MAX,1);

// float [-1 1]
  vtkImageShiftScale *shiftScale_3_0_pre=vtkImageShiftScale::New();
  shiftScale_3_0_pre->SetInputConnection(reader->GetOutputPort());
  shiftScale_3_0_pre->SetScale(0.0078125);
  shiftScale_3_0_pre->SetOutputScalarTypeToFloat();

  shiftScale[3][0]=vtkImageShiftScale::New();
  shiftScale[3][0]->SetInputConnection(shiftScale_3_0_pre->GetOutputPort());
  shiftScale[3][0]->SetShift(-1.0);
  shiftScale[3][0]->SetOutputScalarTypeToFloat();

  color[3][0]=vtkColorTransferFunction::New();
  color[3][0]->AddRGBPoint(-1.0,0,0,1);
  color[3][0]->AddRGBPoint(1.0,0,1,0);

  opacity[3][0]=vtkPiecewiseFunction::New();
  opacity[3][0]->AddPoint(-1.0,0);
  opacity[3][0]->AddPoint(1.0,1);

// double [-1000 3000]
  vtkImageShiftScale *shiftScale_3_1_pre=vtkImageShiftScale::New();
  shiftScale_3_1_pre->SetInputConnection(reader->GetOutputPort());
  shiftScale_3_1_pre->SetScale(15.625);
  shiftScale_3_1_pre->SetOutputScalarTypeToDouble();

  shiftScale[3][1]=vtkImageShiftScale::New();
  shiftScale[3][1]->SetInputConnection(shiftScale_3_1_pre->GetOutputPort());
  shiftScale[3][1]->SetShift(-1000);
  shiftScale[3][1]->SetOutputScalarTypeToDouble();

  color[3][1]=vtkColorTransferFunction::New();
  color[3][1]->AddRGBPoint(-1000,0,0,1);
  color[3][1]->AddRGBPoint(3000,0,1,0);

  opacity[3][1]=vtkPiecewiseFunction::New();
  opacity[3][1]->AddPoint(-1000,0);
  opacity[3][1]->AddPoint(3000,1);

  vtkRenderer *ren1=vtkRenderer::New();
  vtkRenderWindow *renWin=vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  renWin->SetSize(600,300);
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();

  vtkGPUVolumeRayCastMapper *volumeMapper[4][2];
  vtkVolumeProperty *volumeProperty[4][2];
  vtkVolume *volume[4][2];
  vtkTransform *userMatrix[4][2];

  int i=0;
  while(i<4)
    {
    int j=0;
    while(j<2)
      {
      volumeMapper[i][j]=vtkGPUVolumeRayCastMapper::New();
      volumeMapper[i][j]->SetBlendModeToMaximumIntensity();
      volumeMapper[i][j]->SetInputConnection(
        shiftScale[i][j]->GetOutputPort());

      volumeProperty[i][j]=vtkVolumeProperty::New();
      volumeProperty[i][j]->SetColor(color[i][j]);
      volumeProperty[i][j]->SetScalarOpacity(opacity[i][j]);
      volumeProperty[i][j]->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

      volume[i][j]=vtkVolume::New();
      volume[i][j]->SetMapper(volumeMapper[i][j]);
      volume[i][j]->SetProperty(volumeProperty[i][j]);

      userMatrix[i][j]=vtkTransform::New();
      userMatrix[i][j]->PostMultiply();
      userMatrix[i][j]->Identity();
      userMatrix[i][j]->Translate(i*120,j*120,0);

      volume[i][j]->SetUserTransform(userMatrix[i][j]);
      ren1->AddViewProp(volume[i][j]);
      ++j;
      }
    ++i;
    }

  int valid=volumeMapper[0][1]->IsRenderSupported(renWin,volumeProperty[0][1]);

  int retVal;
  if(valid)
    {
    iren->Initialize();
    ren1->SetBackground(0.1,0.4,0.2);
    ren1->ResetCamera();
    ren1->GetActiveCamera()->Zoom(2.0);
    renWin->Render();

    retVal = vtkTesting::Test(argc, argv, renWin, 75);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }
    }
  else
    {
    retVal=vtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
    }

  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  shiftScale_3_1_pre->Delete();
  shiftScale_3_0_pre->Delete();
  i=0;
  while(i<4)
    {
    int j=0;
    while(j<2)
      {
      volumeMapper[i][j]->Delete();
      volumeProperty[i][j]->Delete();
      volume[i][j]->Delete();
      userMatrix[i][j]->Delete();
      shiftScale[i][j]->Delete();
      color[i][j]->Delete();
      opacity[i][j]->Delete();
      ++j;
      }
    ++i;
    }
  reader->Delete();

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
