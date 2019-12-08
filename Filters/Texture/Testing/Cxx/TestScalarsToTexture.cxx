/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarsToTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkColorTransferFunction.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToTextureFilter.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

//----------------------------------------------------------------------------
int TestScalarsToTexture(int argc, char* argv[])
{
  vtkNew<vtkXMLPolyDataReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can_slice.vtp");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkColorTransferFunction> stc;
  stc->SetVectorModeToMagnitude();
  stc->SetColorSpaceToDiverging();
  stc->AddRGBPoint(0.0, 59. / 255., 76. / 255., 192. / 255.);
  stc->AddRGBPoint(7.0e6, 221. / 255., 221. / 255., 221. / 255.);
  stc->AddRGBPoint(1.4e7, 180. / 255., 4. / 255., 38. / 255.);
  stc->Build();

  vtkNew<vtkScalarsToTextureFilter> stt;
  stt->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ACCL");
  stt->SetTextureDimensions(256, 256);
  stt->SetTransferFunction(stc);
  stt->UseTransferFunctionOn();
  stt->SetInputConnection(reader->GetOutputPort());

  // render texture
  vtkNew<vtkImageActor> actor;
  actor->GetMapper()->SetInputConnection(stt->GetOutputPort(1));

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  renderer->AddActor(actor);
  renderer->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
