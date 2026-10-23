// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Arrays a Viskores filter computes on a GPU come back as vtkmDataArrays that
// still live there, so the next Viskores filter reads them in place instead
// of each filter moving its output to the host and the next moving it back.
// Arrays computed on a host device come back as ordinary VTK arrays.

#include "vtkCollection.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMemoryDescriptor.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"

#include "vtkmContour.h"
#include "vtkmDataArrayUtilities.h"
#include "vtkmPointElevation.h"

#include <viskores/cont/DeviceAdapterTag.h>

#include <iostream>
#include <string>

namespace
{
// The memory space an array's storage occupies now, without moving it.
std::string MemorySpaceOf(vtkDataArray* array)
{
  vtkSmartPointer<vtkCollection> descriptors;
  descriptors.TakeReference(array->NewMemoryDescriptors());
  auto* descriptor = vtkMemoryDescriptor::SafeDownCast(descriptors->GetItemAsObject(0));
  return descriptor ? descriptor->GetMemorySpace() : "<none>";
}

bool Check(vtkDataArray* array, const char* what, const char* expectedSpace)
{
  if (!array)
  {
    std::cerr << what << ": missing\n";
    return false;
  }
  const bool wrapped = array->GetArrayType() == vtkArrayTypes::VTKM_DATA_ARRAY;
  const std::string space = MemorySpaceOf(array);
  const bool onHost = std::string(expectedSpace) == "host";
  if (wrapped == onHost || space != expectedSpace)
  {
    std::cerr << what << ": expected " << (onHost ? "a VTK array" : "a vtkmDataArray") << " in \""
              << expectedSpace << "\", got " << array->GetClassName() << " in \"" << space
              << "\"\n";
    return false;
  }
  return true;
}
}

int TestVTKMDeviceResidentOutput(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkmPointElevation> elevation;
  elevation->SetInputConnection(wavelet->GetOutputPort());
  elevation->SetLowPoint(-10, -10, -10);
  elevation->SetHighPoint(10, 10, 10);
  elevation->ForceVTKmOn();

  vtkNew<vtkmContour> contour;
  contour->SetInputConnection(elevation->GetOutputPort());
  contour->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "elevation");
  contour->SetValue(0, 0.5);
  contour->ComputeScalarsOn();
  contour->ForceVTKmOn();
  contour->Update();

  // Where filter output should be: on the GPU when Viskores runs on one, on
  // the host otherwise. Asked after the first filter ran, since that is what
  // initializes Viskores and applies any device the user forced.
  const char* deviceSpace = vtkmDataArrayUtilities::GetDeviceMemorySpaceName();
  const bool gpu = deviceSpace &&
    (vtkmDataArrayUtilities::IsDeviceAdapterAvailable(VISKORES_DEVICE_ADAPTER_CUDA) ||
      vtkmDataArrayUtilities::IsDeviceAdapterAvailable(VISKORES_DEVICE_ADAPTER_KOKKOS));
  const char* expectedSpace = gpu ? deviceSpace : "host";
  std::cout << "Expecting filter output in \"" << expectedSpace << "\"\n";

  auto* elevated = vtkImageData::SafeDownCast(elevation->GetOutput());
  auto* contoured = contour->GetOutput();
  if (contoured->GetNumberOfPoints() == 0)
  {
    std::cerr << "Contour is empty\n";
    return EXIT_FAILURE;
  }

  bool ok = true;
  ok &= Check(elevated->GetPointData()->GetArray("elevation"), "elevation", expectedSpace);
  ok &= Check(contoured->GetPoints()->GetData(), "contour points", expectedSpace);
  ok &= Check(contoured->GetPointData()->GetArray("elevation"), "contour scalars", expectedSpace);
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
