// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageRGBToXYZ.h"
#include "vtkImageShiftScale.h"
#include "vtkImageXYZToLAB.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkTestUtilities.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"

int TestRGBToLAB(int argc, char* argv[])
{
  char* input = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk.png");
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/rgb2lab.vti");

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  vtkNew<vtkPNGReader> inputReader;
  inputReader->SetFileName(input);

  vtkNew<vtkImageShiftScale> normalizer;
  normalizer->SetScale(1.0 / 255);

  vtkNew<vtkImageRGBToXYZ> rgb2xyz;
  vtkNew<vtkImageXYZToLAB> xyz2lab;

  normalizer->SetInputConnection(inputReader->GetOutputPort());
  rgb2xyz->SetInputConnection(normalizer->GetOutputPort());
  xyz2lab->SetInputConnection(rgb2xyz->GetOutputPort());

  xyz2lab->Update();

  return vtkTestUtilities::CompareDataObjects(
           xyz2lab->GetOutputDataObject(0), reader->GetOutputDataObject(0))
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
