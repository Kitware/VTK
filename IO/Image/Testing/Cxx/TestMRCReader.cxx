/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMRCReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkImageMathematics.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkMRCReader.h"

#include <string>

static const char *dispfile = "Data/mrc/emd_1056.mrc";

static void TestDisplay(vtkRenderWindow *renwin, const char *infile)
{
  vtkNew<vtkMRCReader> reader;

  reader->SetFileName(infile);
  reader->Update();

  int size[3];
  double center[3], spacing[3];
  reader->GetOutput()->GetDimensions(size);
  reader->GetOutput()->GetCenter(center);
  reader->GetOutput()->GetSpacing(spacing);
  double center1[3] = { center[0], center[1], center[2] };
  double center2[3] = { center[0], center[1], center[2] };
  if (size[2] % 2 == 1)
  {
    center1[2] += 0.5*spacing[2];
  }
  if (size[0] % 2 == 1)
  {
    center2[0] += 0.5*spacing[0];
  }
  double vrange[2];
  reader->GetOutput()->GetScalarRange(vrange);

  vtkNew<vtkImageSliceMapper> map1;
  map1->BorderOn();
  map1->SliceAtFocalPointOn();
  map1->SliceFacesCameraOn();
  map1->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkImageSlice> slice1;
  slice1->SetMapper(map1.GetPointer());
  slice1->GetProperty()->SetColorWindow(vrange[1]-vrange[0]);
  slice1->GetProperty()->SetColorLevel(0.5*(vrange[0]+vrange[1]));

  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0,0,1.0,1.0);

  ren1->AddViewProp(slice1.GetPointer());

  vtkCamera *cam1 = ren1->GetActiveCamera();
  cam1->ParallelProjectionOn();
  cam1->SetParallelScale(0.5*spacing[1]*size[1]);
  cam1->SetFocalPoint(center1[0], center1[1], center1[2]);
  cam1->SetPosition(center1[0], center1[1], center1[2] - 100.0);

  renwin->SetSize(size[0], size[1]);
  renwin->AddRenderer(ren1.GetPointer());
};

int TestMRCReader(int argc, char *argv[])
{
  // perform the display test
  char *infile =
    vtkTestUtilities::ExpandDataFileName(argc, argv, dispfile);
  if (!infile)
  {
    cerr << "Could not locate input file " << dispfile << "\n";
    return 1;
  }
  std::string inpath = infile;
  delete [] infile;

  vtkNew<vtkRenderWindow> renwin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin.GetPointer());

  TestDisplay(renwin.GetPointer(), inpath.c_str());

  int retVal = vtkRegressionTestImage(renwin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renwin->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal != vtkRegressionTester::PASSED);
}
