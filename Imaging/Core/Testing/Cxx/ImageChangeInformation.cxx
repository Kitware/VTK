// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageChangeInformation.h"
#include "vtkImageData.h"
#include "vtkImageImport.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix3x3.h"
#include "vtkNew.h"

#include <vector>

// Test functions
namespace
{

// Print "<text>: (a, b, c) != (x, y, z)"
template <class T>
void PrintError(const char* text, const T* x, const T* y, int size)
{
  std::cout << text << " ";

  const T* v = x;
  for (int j = 0; j < 2; ++j)
  {
    if (j != 0)
    {
      std::cout << " != ";
    }

    const char* delim = "";
    std::cout << "(";
    for (int i = 0; i < size; ++i)
    {
      std::cout << delim << v[i];
      delim = ", ";
    }
    std::cout << ")";
    v = y;
  }

  std::cout << std::endl;
}

// Compare vectors, print error message if not equal
template <class T>
bool CompareVector(const char* text, const T* x, const T* y, int size)
{
  for (int i = 0; i < size; ++i)
  {
    if (x[i] != y[i])
    {
      PrintError(text, x, y, size);
      return false;
    }
  }

  return true;
}

// Compare vectors with a small tolerance
template <class T>
bool CompareVectorFuzzy(const char* text, const T* x, const T* y, int size)
{
  for (int i = 0; i < size; ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(x[i], y[i]))
    {
      PrintError(text, x, y, size);
      return false;
    }
  }

  return true;
}

// Test passthrough of information (the default behavior)
bool TestPassthrough()
{
  std::cout << "Testing Information Passthrough:" << std::endl;

  const int extent[6] = { 0, 1, 0, 1, 1, 2 };
  const double spacing[3] = { 2.0, 3.0, 4.0 };
  const double direction[9] = { 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0 };
  const double origin[3] = { 10.0, -3.0, 8.0 };

  vtkNew<vtkImageImport> source;
  source->SetDataScalarTypeToFloat();
  source->SetNumberOfScalarComponents(1);
  source->SetWholeExtent(extent);
  source->SetDataExtent(extent);
  std::vector<float> pixels(8);
  source->SetImportVoidPointer(pixels.data());
  source->SetDataSpacing(spacing);
  source->SetDataDirection(direction);
  source->SetDataOrigin(origin);

  vtkNew<vtkImageChangeInformation> change;
  change->SetInputConnection(source->GetOutputPort());
  change->Update();

  vtkImageData* output = change->GetOutput();

  bool success = true;
  success &= CompareVector("Extent:", extent, output->GetExtent(), 6);
  success &= CompareVector("Spacing:", spacing, output->GetSpacing(), 3);
  success &= CompareVector("Direction:", direction, output->GetDirectionMatrix()->GetData(), 9);
  success &= CompareVector("Origin:", origin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

// Test the use of SetInformationInput()
bool TestInformationInput()
{
  std::cout << "Testing SetInformationInput:" << std::endl;

  // the input image
  vtkNew<vtkImageData> input;
  input->SetExtent(0, 1, 0, 1, 0, 1);
  input->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  // the information
  int extent[6] = { 0, 1, 0, 1, -1, 0 };
  const double spacing[3] = { 6.0, 2.0, 5.0 };
  const double direction[9] = { 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0 };
  const double origin[3] = { 9.0, 2.0, 1.5 };

  // the information input
  vtkNew<vtkImageData> info;
  info->SetExtent(extent);
  info->SetSpacing(spacing);
  info->SetDirectionMatrix(direction);
  info->SetOrigin(origin);
  info->AllocateScalars(VTK_FLOAT, 1);

  // use image from 1st input, but information from 2nd input
  vtkNew<vtkImageChangeInformation> change;
  change->SetInputData(input);
  change->SetInformationInputData(info);
  change->Update();

  vtkImageData* output = change->GetOutput();

  bool success = true;
  success &= CompareVector("Extent:", extent, output->GetExtent(), 6);
  success &= CompareVector("Spacing:", spacing, output->GetSpacing(), 3);
  success &= CompareVector("Direction:", direction, output->GetDirectionMatrix()->GetData(), 9);
  success &= CompareVector("Origin:", origin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

// Test manually setting the information
bool TestSetInformation()
{
  std::cout << "Testing Set Methods:" << std::endl;

  // the input image
  vtkNew<vtkImageData> input;
  input->SetExtent(0, 1, 0, 1, 0, 1);
  input->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  // the information
  const int extent[6] = { 0, 1, 0, 1, 2, 3 };
  const double spacing[3] = { 6.5, 2.5, 5.5 };
  const double direction[9] = { 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, 0.0, -1.0, 0.0 };
  const double origin[3] = { 9.0, 2.0, 1.5 };

  // manually set the new information
  vtkNew<vtkImageChangeInformation> change;
  change->SetInputData(input);
  change->SetOutputExtentStart(extent[0], extent[2], extent[4]);
  change->SetOutputSpacing(spacing);
  change->SetOutputDirection(direction);
  change->SetOutputOrigin(origin);
  change->Update();

  vtkImageData* output = change->GetOutput();

  bool success = true;
  success &= CompareVector("Extent:", extent, output->GetExtent(), 6);
  success &= CompareVector("Spacing:", spacing, output->GetSpacing(), 3);
  success &= CompareVector("Direction:", direction, output->GetDirectionMatrix()->GetData(), 9);
  success &= CompareVector("Origin:", origin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

// Test adjusting the information
bool TestAdjustInformation()
{
  std::cout << "Testing Scale and Translation Methods:" << std::endl;

  // the input image
  vtkNew<vtkImageData> input;
  input->SetExtent(0, 1, 0, 1, 0, 1);
  input->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  // the information
  const int extent[6] = { 3, 4, -1, 0, 2, 3 };
  const double spacing[3] = { 6.5, 2.5, 5.5 };
  const double origin[3] = { 9.0, 2.0, 1.5 };

  // adjust the information via translation and scaling
  vtkNew<vtkImageChangeInformation> change;
  change->SetInputData(input);
  change->SetExtentTranslation(extent[0], extent[2], extent[4]);
  change->SetSpacingScale(spacing);
  change->SetOriginTranslation(origin);
  change->Update();

  vtkImageData* output = change->GetOutput();

  bool success = true;
  success &= CompareVector("Extent:", extent, output->GetExtent(), 6);
  success &= CompareVectorFuzzy("Spacing:", spacing, output->GetSpacing(), 3);
  success &= CompareVectorFuzzy("Origin:", origin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

// Test centering the image
bool TestCenter()
{
  std::cout << "Testing Scale and Translation Methods:" << std::endl;

  // the input image
  vtkNew<vtkImageData> input;
  input->SetExtent(0, 1, 0, 1, 0, 1);
  input->SetSpacing(1.5, 1.5, 1.5);
  input->SetDirectionMatrix(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0);
  input->SetOrigin(3.6, 8.4, -1.0);
  input->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  // request centering of the image on (0.0,0.0,0.0)
  vtkNew<vtkImageChangeInformation> change;
  change->SetInputData(input);
  change->CenterImageOn();
  change->Update();

  vtkImageData* output = change->GetOutput();
  double center[3];
  output->GetCenter(center);
  const double expectedCenter[3] = { 0.0, 0.0, 0.0 };
  bool success = CompareVectorFuzzy("Center:", expectedCenter, center, 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

}

// Driver Function
int ImageChangeInformation(int, char*[])
{
  bool success = true;

  success &= TestPassthrough();
  success &= TestInformationInput();
  success &= TestSetInformation();
  success &= TestAdjustInformation();
  success &= TestCenter();

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
