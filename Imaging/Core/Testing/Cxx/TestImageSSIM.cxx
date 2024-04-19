// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageRGBToXYZ.h"
#include "vtkImageSSIM.h"
#include "vtkImageShiftScale.h"
#include "vtkImageXYZToLAB.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkTestUtilities.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"

int TestImageSSIM(int argc, char* argv[])
{
  auto createPipeline = [](std::string&& name) {
    vtkNew<vtkPNGReader> reader;
    reader->SetFileName(name.c_str());

    vtkNew<vtkImageShiftScale> normalizer;
    normalizer->SetScale(1.0 / 255);
    vtkNew<vtkImageRGBToXYZ> rgb2xyz;
    vtkNew<vtkImageXYZToLAB> xyz2lab;

    normalizer->SetInputConnection(reader->GetOutputPort());
    rgb2xyz->SetInputConnection(normalizer->GetOutputPort());
    xyz2lab->SetInputConnection(rgb2xyz->GetOutputPort());

    return xyz2lab;
  };

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ssim.vti");

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  auto input1 =
    createPipeline(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ImageDiff1.png"));
  auto input2 =
    createPipeline(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ImageDiff2.png"));

  vtkNew<vtkImageSSIM> ssim;
  ssim->SetInputConnection(0, input1->GetOutputPort());
  ssim->SetInputConnection(1, input2->GetOutputPort());
  ssim->SetInputToLab();
  ssim->Update();

  return vtkTestUtilities::CompareDataObjects(
           ssim->GetOutputDataObject(0), reader->GetOutputDataObject(0))
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
