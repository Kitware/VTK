// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The purpose of this test is to check that the following pipeline
// information is passed from the Input to the Output for VTK image
// filters: SPACING, DIRECTION, ORIGIN
//
// Some common filters are tested, but the testing is not exhaustive.

#include "vtkFloatArray.h"
#include "vtkImageAlgorithm.h"
#include "vtkImageCityBlockDistance.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImageResample.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix3x3.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkScalarsToColors.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

// Test functions
namespace
{
const double TestSpacing[3] = { 1.2, 3.8, 1.0 };
const double TestDirection[9] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0 };
const double TestOrigin[3] = { -1.8, 0.0, 20.0 };
const int TestExtent[6] = { 0, 9, -3, 3, 1, 8 };

// A simple VTK image source that provides DIRECTION
class vtkImageInformationSource : public vtkImageAlgorithm
{
public:
  static vtkImageInformationSource* New();
  vtkTypeMacro(vtkImageInformationSource, vtkImageAlgorithm);

protected:
  vtkImageInformationSource() { this->SetNumberOfInputPorts(0); }
  ~vtkImageInformationSource() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void ExecuteDataWithInformation(vtkDataObject* data, vtkInformation* outInfo) override;

private:
  vtkImageInformationSource(const vtkImageInformationSource&) = delete;
  void operator=(const vtkImageInformationSource&) = delete;
};

vtkStandardNewMacro(vtkImageInformationSource);

int vtkImageInformationSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::SPACING(), TestSpacing, 3);
  outInfo->Set(vtkDataObject::DIRECTION(), TestDirection, 9);
  outInfo->Set(vtkDataObject::ORIGIN(), TestOrigin, 3);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), TestExtent, 6);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_SHORT, 1);

  return 1;
}

void vtkImageInformationSource::ExecuteDataWithInformation(
  vtkDataObject* output, vtkInformation* outInfo)
{
  vtkImageData* data = this->AllocateOutputData(output, outInfo);
  int* outExt = data->GetExtent();
  short* outPtr = static_cast<short*>(data->GetScalarPointerForExtent(outExt));
  vtkIdType size = 1;
  for (int i = 0; i < 3; i++)
  {
    size *= outExt[2 * i + 1] - outExt[2 * i] + 1;
  }
  std::fill(outPtr, outPtr + size, 0.0);
}

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

// Test information passthrough for core filter vtkImageResample
bool TestResamplePassthrough()
{
  std::cout << "Test Information Passthrough for vtkImageResample:" << std::endl;

  vtkNew<vtkImageInformationSource> source;

  vtkNew<vtkImageResample> resample;
  resample->SetInputConnection(source->GetOutputPort());
  resample->Update();

  vtkInformation* outputInfo = resample->GetOutputInformation(0);
  vtkImageData* output = resample->GetOutput();

  bool success = true;
  success &= CompareVector("WholeExtent:", TestExtent,
    outputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  success &= CompareVector("Spacing:", TestSpacing, output->GetSpacing(), 3);
  success &= CompareVector("Direction:", TestDirection, output->GetDirectionMatrix()->GetData(), 9);
  success &= CompareVector("Origin:", TestOrigin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

// Test information passthrough for core filter vtkImageMapToColors
bool TestMapToColorsPassthrough()
{
  std::cout << "Test Information Passthrough for vtkImageMapToColors:" << std::endl;

  vtkNew<vtkImageInformationSource> source;

  vtkNew<vtkScalarsToColors> lut;

  vtkNew<vtkImageMapToColors> colors;
  colors->SetLookupTable(lut);
  colors->SetInputConnection(source->GetOutputPort());
  colors->Update();

  vtkInformation* outputInfo = colors->GetOutputInformation(0);
  vtkImageData* output = colors->GetOutput();

  bool success = true;
  success &= CompareVector("WholeExtent:", TestExtent,
    outputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  success &= CompareVector("Spacing:", TestSpacing, output->GetSpacing(), 3);
  success &= CompareVector("Direction:", TestDirection, output->GetDirectionMatrix()->GetData(), 9);
  success &= CompareVector("Origin:", TestOrigin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

// This tests vtkImageIterateFilter, the base class of separable filters
bool TestSeparablePassthrough()
{
  std::cout << "Test Information Passthrough for vtkImageIterateFilter:" << std::endl;

  vtkNew<vtkImageInformationSource> source;

  vtkNew<vtkImageCityBlockDistance> distance;
  distance->SetInputConnection(source->GetOutputPort());
  distance->Update();

  vtkInformation* outputInfo = distance->GetOutputInformation(0);
  vtkImageData* output = distance->GetOutput();

  bool success = true;
  success &= CompareVector("WholeExtent:", TestExtent,
    outputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  success &= CompareVector("Spacing:", TestSpacing, output->GetSpacing(), 3);
  success &= CompareVector("Direction:", TestDirection, output->GetDirectionMatrix()->GetData(), 9);
  success &= CompareVector("Origin:", TestOrigin, output->GetOrigin(), 3);

  if (success)
  {
    std::cout << "Success!" << std::endl;
  }

  return success;
}

}

// Driver Function
int ImagePassInformation(int, char*[])
{
  bool success = true;

  success &= TestResamplePassthrough();
  success &= TestMapToColorsPassthrough();
  success &= TestSeparablePassthrough();

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
