// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredPointArray.h"
#include "vtkStructuredPointBackend.txx"

#include "vtkArrayDispatch.h"

namespace
{
//------------------------------------------------------------------------------
template <typename ValueType, bool UseDirMatrix>
struct StructuredPointsWorker
{
  template <typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ>
  void operator()(ArrayTypeX* arrayX, ArrayTypeY* arrayY, ArrayTypeZ* arrayZ,
    vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>>>& structuredPointArray,
    int extent[6], int dataDescription, double dirMatrix[9])
  {
    std::shared_ptr<vtkStructuredPointBackend<ValueType>> backend;
    switch (dataDescription)
    {
      case 9 /*VTK_EMPTY*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          9, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 1 /*VTK_SINGLE_POINT*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          1, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 2 /*VTK_X_LINE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          2, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 3 /*VTK_Y_LINE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          3, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 4 /*VTK_Z_LINE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          4, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 5 /*VTK_XY_PLANE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          5, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 6 /*VTK_YZ_PLANE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          6, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 7 /*VTK_XZ_PLANE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          7, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 8 /*VTK_XYZ_GRID*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          8, UseDirMatrix>;
        backend = std::make_shared<TBackend>(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      default:
      {
        vtkGenericWarningMacro("Execute: Unknown data description" << dataDescription);
        break;
      }
    }
    structuredPointArray->SetBackend(backend);
  }
};
} // end anon namespace

//-----------------------------------------------------------------------
namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>>> CreateStructuredPointArray(
  vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords, int extent[6],
  int dataDescription, double dirMatrix[9])
{
  assert(xCoords && yCoords && zCoords && extent && dirMatrix);
  const bool isIdentity = (dirMatrix[0] == 1.0 && dirMatrix[4] == 1.0 && dirMatrix[8] == 1.0 &&
    dirMatrix[1] == 0.0 && dirMatrix[2] == 0.0 && dirMatrix[3] == 0.0 && dirMatrix[5] == 0.0 &&
    dirMatrix[6] == 0.0 && dirMatrix[7] == 0.0);
  int dim[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 };
  auto structuredPointArray =
    vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>>>::New();
  structuredPointArray->SetNumberOfComponents(3);
  const auto numPoints = static_cast<vtkIdType>(dim[0]) * dim[1] * dim[2];
  structuredPointArray->SetNumberOfTuples(numPoints);
  if (isIdentity)
  {
    StructuredPointsWorker<ValueType, false /*UseDirMatrix=*/> worker;
    using Dispatcher = vtkArrayDispatch::Dispatch3BySameValueType<vtkArrayDispatch::Reals>;
    if (!Dispatcher::Execute(xCoords, yCoords, zCoords, worker, structuredPointArray, extent,
          dataDescription, dirMatrix))
    {
      worker(xCoords, yCoords, zCoords, structuredPointArray, extent, dataDescription, dirMatrix);
    }
  }
  else
  {
    StructuredPointsWorker<ValueType, true /*UseDirMatrix=*/> worker;
    using Dispatcher = vtkArrayDispatch::Dispatch3BySameValueType<vtkArrayDispatch::Reals>;
    if (!Dispatcher::Execute(xCoords, yCoords, zCoords, worker, structuredPointArray, extent,
          dataDescription, dirMatrix))
    {
      worker(xCoords, yCoords, zCoords, structuredPointArray, extent, dataDescription, dirMatrix);
    }
  }
  return structuredPointArray;
}
VTK_ABI_NAMESPACE_END
}
