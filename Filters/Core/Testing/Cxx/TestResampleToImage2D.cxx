/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleToImage2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This is just a simple test. vtkResampleToImage internally uses
// vtkProbeFilter, which is tested thoroughly in other tests.

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkResampleToImage.h"
#include "vtkTestUtilities.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestResampleToImage2D(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/delaunay3d.vtu");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkResampleToImage> resample;
  resample->UseInputBoundsOff();
  resample->SetInputConnection(reader->GetOutputPort());

  double* range = nullptr;

  // test on X
  resample->SetSamplingBounds(0.0, 0.0, -10.0, 10.0, -10.0, 10.0);
  resample->SetSamplingDimensions(1, 100, 100);
  resample->Update();
  range = resample->GetOutput()->GetPointData()->GetArray("BrownianVectors")->GetRange();
  if (range[1] - range[0] < 0.01)
  {
    cerr << "Error resampling along X" << endl;
  }

  // test on Y
  resample->SetSamplingBounds(-10.0, 10.0, 0.0, 0.0, -10.0, 10.0);
  resample->SetSamplingDimensions(100, 1, 100);
  resample->Update();
  range = resample->GetOutput()->GetPointData()->GetArray("BrownianVectors")->GetRange();
  if (range[1] - range[0] < 0.01)
  {
    cerr << "Error resampling along Y" << endl;
  }

  // test on Z
  resample->SetSamplingBounds(-10.0, 10.0, -10.0, 10.0, 0.0, 0.0);
  resample->SetSamplingDimensions(100, 100, 1);
  resample->Update();
  range = resample->GetOutput()->GetPointData()->GetArray("BrownianVectors")->GetRange();
  if (range[1] - range[0] < 0.01)
  {
    cerr << "Error resampling along Z" << endl;
  }

  return EXIT_SUCCESS;
}
