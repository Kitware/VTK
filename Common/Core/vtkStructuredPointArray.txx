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
    vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>,
      vtkArrayTypes::StructuredPointArray>>& structuredPointArray,
    int extent[6], int dataDescription, double dirMatrix[9])
  {
    // Using a raw pointer to the base class here is important so we don't instantiate
    // std::shared_ptr (+ control block and deleter) for each possible backend type.
    vtkStructuredPointBackend<ValueType>* backend{};

    switch (dataDescription)
    {
      case 9 /*VTK_STRUCTURED_EMPTY*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          9, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 1 /*VTK_STRUCTURED_SINGLE_POINT*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          1, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 2 /*VTK_STRUCTURED_X_LINE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          2, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 3 /*VTK_STRUCTURED_Y_LINE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          3, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 4 /*VTK_STRUCTURED_Z_LINE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          4, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 5 /*VTK_STRUCTURED_XY_PLANE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          5, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 6 /*VTK_STRUCTURED_YZ_PLANE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          6, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 7 /*VTK_STRUCTURED_XZ_PLANE*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          7, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      case 8 /*VTK_STRUCTURED_XYZ_GRID*/:
      {
        using TBackend = vtkStructuredTPointBackend<ValueType, ArrayTypeX, ArrayTypeY, ArrayTypeZ,
          8, UseDirMatrix>;
        backend = new TBackend(arrayX, arrayY, arrayZ, extent, dirMatrix);
        break;
      }
      default:
      {
        vtkGenericWarningMacro("Execute: Unknown data description" << dataDescription);
        break;
      }
    }

    structuredPointArray->SetBackend(
      std::shared_ptr<vtkStructuredPointBackend<ValueType>>{ backend });
  }
};
} // end anon namespace

//-----------------------------------------------------------------------
namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
vtkSmartPointer<
  vtkImplicitArray<vtkStructuredPointBackend<ValueType>, vtkArrayTypes::StructuredPointArray>>
CreateStructuredPointArray(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,
  int extent[6], int dataDescription, double dirMatrix[9])
{
  assert(xCoords && yCoords && zCoords && extent && dirMatrix);
  const bool isIdentity = (dirMatrix[0] == 1.0 && dirMatrix[4] == 1.0 && dirMatrix[8] == 1.0 &&
    dirMatrix[1] == 0.0 && dirMatrix[2] == 0.0 && dirMatrix[3] == 0.0 && dirMatrix[5] == 0.0 &&
    dirMatrix[6] == 0.0 && dirMatrix[7] == 0.0);
  int dim[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 };
  auto structuredPointArray = vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>,
    vtkArrayTypes::StructuredPointArray>>::New();
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
