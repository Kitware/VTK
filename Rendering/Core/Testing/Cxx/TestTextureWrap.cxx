/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextureWrap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test for the different texture wrap modes

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexture.h"

int TestTextureWrap(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");

  vtkNew<vtkJPEGReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(800, 200); // Intentional NPOT size

  vtkNew<vtkPlaneSource> planeSource;
  planeSource->Update();
  vtkPolyData* plane = planeSource->GetOutput();

  vtkFloatArray* tcoord = vtkFloatArray::SafeDownCast(plane->GetPointData()->GetTCoords());

  for (int i = 0; i < tcoord->GetNumberOfTuples(); ++i)
  {
    float tmp[2];
    tcoord->GetTypedTuple(i, tmp);
    for (int j = 0; j < 2; ++j)
      tmp[j] = 2 * tmp[j] - 0.5;
    tcoord->SetTuple2(i, tmp[0], tmp[1]);
  }

  vtkNew<vtkRenderer> ren[4];
  vtkNew<vtkTexture> texture[4];
  vtkNew<vtkPolyDataMapper> mapper[4];
  vtkNew<vtkActor> actor[4];

  for (int i = 0; i < 4; ++i)
  {
    ren[i]->SetViewport(i * 0.25, 0, i * 0.25 + 0.25, 1);
    renWin->AddRenderer(ren[i]);

    texture[i]->SetInputConnection(reader->GetOutputPort());
    texture[i]->SetBorderColor(0.5, 0.5, 0.5, 0.5);
    texture[i]->InterpolateOn();
    texture[i]->SetWrap(i);

    mapper[i]->SetInputData(plane);
    actor[i]->SetMapper(mapper[i]);
    actor[i]->SetTexture(texture[i]);
    ren[i]->AddActor(actor[i]);
    ren[i]->ResetCamera();
    ren[i]->GetActiveCamera()->Zoom(1.4);
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  delete[] fname;
  return !retVal;
}
