// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredPointArray.h"

#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"

#ifdef VTK_DISPATCH_STRUCTURED_POINT_ARRAYS
#include "vtkArrayDispatch.h"
#endif // VTK_DISPATCH_STRUCTURED_POINT_ARRAYS

#include <cstdlib>
#include <numeric>
#include <random>

#ifdef VTK_DISPATCH_STRUCTURED_POINT_ARRAYS
namespace
{
template <typename ValueType>
struct DispatcherCheckerWorker
{
  template <typename PointsArray>
  void operator()(PointsArray* vtkNotUsed(pointsArray))
  {
  }
};
}
#endif // VTK_DISPATCH_STRUCTURED_POINT_ARRAYS

int TestStructuredPointArrayExtent(int extent[6])
{
  int dims[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 };
  const auto dataDescription = vtkStructuredData::GetDataDescription(dims);
  vtkNew<vtkDoubleArray> xCoords;
  xCoords->SetNumberOfValues(dims[0]);
  std::iota(xCoords->GetPointer(0), xCoords->GetPointer(dims[0]), double(0));
  vtkNew<vtkDoubleArray> yCoords;
  yCoords->SetNumberOfValues(dims[1]);
  std::iota(yCoords->GetPointer(0), yCoords->GetPointer(dims[1]), double(0));
  vtkNew<vtkDoubleArray> zCoords;
  zCoords->SetNumberOfValues(dims[2]);
  std::iota(zCoords->GetPointer(0), zCoords->GetPointer(dims[2]), double(0));

  double identityMatrix[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
  const vtkSmartPointer<vtkStructuredPointArray<double>> implicitPointArray =
    vtk::CreateStructuredPointArray<double>(
      xCoords, yCoords, zCoords, extent, dataDescription, identityMatrix);

  vtkNew<vtkImageData> image;
  image->SetDimensions(dims);
  image->SetOrigin(0, 0, 0);
  image->SetSpacing(1, 1, 1);

  int ijk[3];
  double point1[3], point2[3];
  for (vtkIdType i = 0; i < image->GetNumberOfPoints(); ++i)
  {
    vtkStructuredData::ComputePointStructuredCoords(i, dims, ijk);
    image->TransformIndexToPhysicalPoint(ijk, point1);
    implicitPointArray->GetTypedTuple(i, point2);
    for (int j = 0; j < 3; ++j)
    {
      if (vtkMath::Round(point1[j]) != vtkMath::Round(point2[j]))
      {
        vtkErrorWithObjectMacro(nullptr,
          "Point mismatch at index " << i << " and coordinate " << j << " of " << point1[j]
                                     << " and " << point2[j]);
        return EXIT_FAILURE;
      }
    }
  }
#ifdef VTK_DISPATCH_STRUCTURED_POINT_ARRAYS
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  DispatcherCheckerWorker<double> worker;
  if (!Dispatcher::Execute(implicitPointArray, worker))
  {
    return EXIT_FAILURE;
  }
#endif // VTK_DISPATCH_IMPLICIT_POINT_ARRAYS
  return EXIT_SUCCESS;
}

int TestStructuredPointArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int res = EXIT_SUCCESS;
  int extentX[6] = { 0, 30, 0, 0, 0, 0 };
  res &= TestStructuredPointArrayExtent(extentX);
  int extentY[6] = { 0, 0, 0, 19, 0, 0 };
  res &= TestStructuredPointArrayExtent(extentY);
  int extentZ[6] = { 0, 0, 0, 0, 0, 38 };
  res &= TestStructuredPointArrayExtent(extentZ);
  int extentXZ[6] = { 0, 30, 0, 0, 0, 38 };
  res &= TestStructuredPointArrayExtent(extentXZ);
  int extentYZ[6] = { 0, 0, 0, 19, 0, 38 };
  res &= TestStructuredPointArrayExtent(extentYZ);
  int extentXY[6] = { 0, 30, 0, 19, 0, 0 };
  res &= TestStructuredPointArrayExtent(extentXY);
  int extentXYZ[6] = { 0, 30, 0, 19, 0, 38 };
  res &= TestStructuredPointArrayExtent(extentXYZ);

  return res;
};
