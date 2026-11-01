// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkMultiVariateToRGBA on distributed data gives the same 'rgba'
 * and 'gbc' as on all the data in one process, as the ranges used to
 * normalize the components are reduced across the processes. Each process has
 * a slab of an image, with component ranges that differ between the slabs.
 */

#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkMPIController.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <cmath>
#include <functional>
#include <iostream>

namespace
{
#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Rank " << rank << ", line " << __LINE__ << ": check failed: " #condition         \
              << std::endl;                                                                        \
    return false;                                                                                  \
  }

constexpr int Dimensions[3] = { 21, 5, 3 };
constexpr double Tolerance = 1e-6;

// Returns the image with the x extent [x0, x1] and a 4-component 'mv' array
// that depends on x, so each slab has different component ranges.
vtkSmartPointer<vtkImageData> MakeImage(int x0, int x1)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetExtent(x0, x1, 0, Dimensions[1] - 1, 0, Dimensions[2] - 1);
  vtkNew<vtkDoubleArray> values;
  values->SetName("mv");
  values->SetNumberOfComponents(4);
  values->SetNumberOfTuples(image->GetNumberOfPoints());
  for (vtkIdType i = 0; i < image->GetNumberOfPoints(); ++i)
  {
    double p[3];
    image->GetPoint(i, p);
    values->SetTuple4(i, p[0] + 0.1 * p[1], 20 - p[0] + 2 * p[2], 3 * std::sin(0.7 * p[0]) + p[1],
      0.05 * p[0] * p[0] + p[2]);
  }
  image->GetPointData()->AddArray(values);
  return image;
}

vtkSmartPointer<vtkMultiVariateToRGBA> Run(vtkImageData* image,
  vtkMultiProcessController* controller, const std::function<void(vtkMultiVariateToRGBA*)>& setup)
{
  auto filter = vtkSmartPointer<vtkMultiVariateToRGBA>::New();
  filter->SetController(controller);
  filter->SetInputData(image);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "mv");
  filter->SetStoreGBC(true);
  setup(filter);
  filter->Update();
  return filter;
}

// Checks that 'rgba' and 'gbc' on this process's slab match the ones for
// all the data, computed on this process.
bool MatchesSerial(vtkMultiProcessController* controller, vtkImageData* piece, vtkImageData* whole,
  const std::function<void(vtkMultiVariateToRGBA*)>& setup)
{
  int rank = controller->GetLocalProcessId();
  auto distributed = Run(piece, controller, setup);
  auto serial = Run(whole, nullptr, setup);
  auto* output = vtkImageData::SafeDownCast(distributed->GetOutputDataObject(0));
  auto* expected = vtkImageData::SafeDownCast(serial->GetOutputDataObject(0));
  for (const char* name : { "rgba", "gbc" })
  {
    vtkDataArray* actualArray = output->GetPointData()->GetArray(name);
    vtkDataArray* expectedArray = expected->GetPointData()->GetArray(name);
    CHECK(actualArray && expectedArray);
    for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
    {
      double p[3];
      output->GetPoint(i, p);
      vtkIdType j = expected->FindPoint(p);
      CHECK(j >= 0);
      for (int c = 0; c < actualArray->GetNumberOfComponents(); ++c)
      {
        if (std::fabs(actualArray->GetComponent(i, c) - expectedArray->GetComponent(j, c)) >
          Tolerance)
        {
          std::cerr << "Rank " << rank << ": " << name << " at (" << p[0] << ", " << p[1] << ", "
                    << p[2] << "), component " << c << ": " << actualArray->GetComponent(i, c)
                    << ", expected " << expectedArray->GetComponent(j, c) << std::endl;
          return false;
        }
      }
    }
  }
  return true;
}
}

int TestMultiVariateToRGBAParallel(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  int rank = controller->GetLocalProcessId();
  int size = controller->GetNumberOfProcesses();
  // slabs along x, sharing their boundary points
  int cells = Dimensions[0] - 1;
  vtkSmartPointer<vtkImageData> piece = MakeImage(rank * cells / size, (rank + 1) * cells / size);
  vtkSmartPointer<vtkImageData> whole = MakeImage(0, cells);

  // every test runs on all processes, so a failure does not leave the others waiting
  bool success = MatchesSerial(controller, piece, whole, [](vtkMultiVariateToRGBA*) {});
  success = MatchesSerial(controller, piece, whole,
              [](vtkMultiVariateToRGBA* filter)
              {
                for (int i : { 0, 1, 2, 3 })
                {
                  filter->AddPartitionIndex(i);
                }
                filter->AddPartitionOffset(2);
                filter->AddPartitionOffset(4);
              }) &&
    success;
  success = MatchesSerial(controller, piece, whole,
              [](vtkMultiVariateToRGBA* filter)
              {
                for (int i : { 0, 2, 3 })
                {
                  filter->AddVisibleComponent(i);
                }
              }) &&
    success;
  int localSuccess = success;
  int globalSuccess = 0;
  controller->AllReduce(&localSuccess, &globalSuccess, 1, vtkCommunicator::MIN_OP);
  controller->Finalize();
  return globalSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
