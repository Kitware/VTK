/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkResampleWithDataSet.h"

#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkContourFilter.h"
#include "vtkDataSet.h"
#include "vtkExodusIIReader.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"


enum
{
  TEST_PASSED_RETVAL = 0,
  TEST_FAILED_RETVAL = 1
};

int TestResampleWithDataSet2(int argc, char *argv[])
{
  vtkNew<vtkExodusIIReader> reader;
  char *fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fname);
  delete [] fname;

  reader->UpdateInformation();
  reader->SetObjectArrayStatus(vtkExodusIIReader::NODAL, "VEL", 1);
  reader->Update();

  // based on can.ex2 bounds
  double origin[3] = {-7.8, -1.0, -15};
  double spacing[3] = {0.127, 0.072, 0.084};
  int dims[3] = { 128, 128, 128 };

  vtkNew<vtkImageData> input;
  input->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
  input->SetOrigin(origin);
  input->SetSpacing(spacing);

  vtkNew<vtkResampleWithDataSet> resample;
  resample->SetInputData(input.GetPointer());
  resample->SetSourceConnection(reader->GetOutputPort());
  resample->UpdateTimeStep(0.00199999);

  vtkDataSet *result = static_cast<vtkDataSet*>(resample->GetOutput());


  // Render
  vtkNew<vtkContourFilter> toPoly;
  toPoly->SetInputData(result);
  toPoly->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                 "vtkValidPointMask");
  toPoly->SetValue(0, 0.5);

  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetInputConnection(toPoly->GetOutputPort());
  calculator->AddVectorArrayName("VEL");
  calculator->SetFunction("mag(VEL)");
  calculator->SetResultArrayName("VEL_MAG");
  calculator->Update();


  double range[2];
  calculator->GetOutput()->GetPointData()->GetArray("VEL_MAG")->GetRange(range);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(calculator->GetOutputPort());
  mapper->SetScalarRange(range);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());
  renderer->GetActiveCamera()->SetPosition(0.0, -1.0, 0.0);
  renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
