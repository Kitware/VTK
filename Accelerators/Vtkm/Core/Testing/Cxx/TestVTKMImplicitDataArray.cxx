// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkExtractVOI.h>
#include <vtkImageData.h>
#include <vtkMathUtilities.h>
#include <vtkPointData.h>

#include <vtkmDataArray.h>

#include <viskores/VectorAnalysis.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <array>
#include <functional>

namespace
{

struct TransformFnctr
{
  VISKORES_EXEC_CONT
  double operator()(const viskores::Vec3f& point) const { return viskores::Magnitude(point); }
};

} // namespace

//------------------------------------------------------------------------------
int TestVTKMImplicitDataArray(int, char*[])
{
  viskores::Id dimension = 10;
  viskores::Vec3f boundsMin(0.0f);
  viskores::Vec3f boundsMax(3.0f, 3.0f, 2.0f);
  viskores::Id3 dim3(dimension);
  viskores::Vec3f origin = boundsMin;
  viskores::Vec3f spacing =
    (boundsMax - boundsMin) / viskores::Vec3f(static_cast<viskores::FloatDefault>(dimension));

  vtkNew<vtkImageData> imageData;
  imageData->SetDimensions(dim3[0], dim3[1], dim3[2]);
  imageData->SetSpacing(spacing[0], spacing[1], spacing[2]);
  imageData->SetOrigin(origin[0], origin[1], origin[2]);

  vtkNew<vtkmDataArray<double>> array;
  array->SetVtkmArrayHandle(viskores::cont::make_ArrayHandleTransform(
    viskores::cont::ArrayHandleUniformPointCoordinates(dim3, origin, spacing), TransformFnctr{}));
  imageData->GetPointData()->SetScalars(array);

  vtkNew<vtkExtractVOI> extractor;
  extractor->SetInputData(imageData);
  int dim = dimension;
  int extent[6] = { 0, dim - 1, 0, dim - 1, dim - 1, dim - 1 };
  extractor->SetVOI(extent);
  extractor->Update();
  auto slice = extractor->GetOutput();
  auto outScalars = slice->GetPointData()->GetScalars();
  auto outDataArray = vtkDataArray::SafeDownCast(outScalars);

  if (!outDataArray)
  {
    std::cerr << "Error: no output data array" << std::endl;
  }

  auto nbOfTuples = outDataArray->GetNumberOfTuples();
  auto value1 = outDataArray->GetTuple1(0);
  auto value2 = outDataArray->GetTuple1(dim);
  auto value3 = outDataArray->GetTuple1(2 * dim + 5);
  double range[2];
  outDataArray->GetRange(range);

  if (nbOfTuples != 100)
  {
    std::cerr << "Error: expecting 100 tuples, has " << nbOfTuples << std::endl;
  }

  if (!vtkMathUtilities::NearlyEqual(range[0], 1.8) ||
    !vtkMathUtilities::NearlyEqual(range[1], 4.22137))
  {
    std::cerr << "Error: range should be [1.8, 4.22137], has " << range[0] << " " << range[1]
              << std::endl;
  }

  if (!vtkMathUtilities::NearlyEqual(value1, 1.8))
  {
    std::cerr << "Error: value 0 should be 1.8 has " << value1 << std::endl;
  }

  if (!vtkMathUtilities::NearlyEqual(value2, 1.82483))
  {
    std::cerr << "Error: value 0 should be 1.82483 has " << value1 << std::endl;
  }

  if (!vtkMathUtilities::NearlyEqual(value3, 2.42868))
  {
    std::cerr << "Error: value 0 should be 2.42868 has " << value1 << std::endl;
  }

  return EXIT_SUCCESS;
}
