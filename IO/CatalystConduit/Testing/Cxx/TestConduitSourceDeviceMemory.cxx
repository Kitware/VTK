// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <array>
#include <iterator>
#include <vtkXMLUniformGridAMRWriter.h>

#include "viskores/Flags.h"
#include "viskores/Math.h"
#include "viskores/Types.h"
#include "viskores/cont/ArrayCopy.h"
#include "viskores/cont/ArrayHandle.h"
#include "viskores/cont/ArrayHandleBasic.h"
#include "viskores/cont/ArrayHandleCounting.h"
#include "viskores/cont/DeviceAdapterTag.h"
#include "viskores/cont/ErrorBadValue.h"
#include "viskores/cont/Initialize.h"
#include "viskores/cont/Invoker.h"
#include "viskores/cont/RuntimeDeviceInformation.h"
#include "viskores/cont/RuntimeDeviceTracker.h"
#include "viskores/cont/Token.h"
#include "viskores/cont/UnknownArrayHandle.h"
#include "viskores/cont/cuda/internal/CudaAllocator.h"
#include "viskores/worklet/WorkletMapField.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkConduitArrayUtilities.h"
#include "vtkConduitSource.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "Grid.hxx"
#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#define VERIFY(x, ...)                                                                             \
  if ((x) == false)                                                                                \
  {                                                                                                \
    vtkLogF(ERROR, __VA_ARGS__);                                                                   \
    return false;                                                                                  \
  }

#define SCOPED_CUDA_DISABLE_MANAGED_MEMORY                                                         \
  ScopedCudaDisableManagedMemory _scoped_cuda_disable_managed_mem;                                 \
  (void)_scoped_cuda_disable_managed_mem

#define SCOPED_RUNTIME_DEVICE_SELECTOR(memory_space)                                               \
  viskores::cont::ScopedRuntimeDeviceTracker deviceTracker(                                        \
    viskores::cont::make_DeviceAdapterId(memory_space));                                           \
  (void)deviceTracker

namespace
{

vtkSmartPointer<vtkDataObject> Convert(const conduit_cpp::Node& node)
{
  vtkNew<vtkConduitSource> source;
  source->SetNode(conduit_cpp::c_node(&node));
  source->Update();
  return source->GetOutputDataObject(0);
}

struct ScopedCudaDisableManagedMemory
{
  ScopedCudaDisableManagedMemory()
  {
    using namespace viskores::cont::cuda::internal;
    if (CudaAllocator::UsingManagedMemory())
    {
      this->WasManagedMemoryEnabled = true;
      CudaAllocator::ForceManagedMemoryOff();
    }
  }
  ~ScopedCudaDisableManagedMemory()
  {
    if (this->WasManagedMemoryEnabled)
    {
      viskores::cont::cuda::internal::CudaAllocator::ForceManagedMemoryOn();
    }
  }

private:
  bool WasManagedMemoryEnabled = false;
};

// Helper worklets used to populate coordinates/topology on the device.
struct RectilinearCoordsWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_CONT RectilinearCoordsWorklet(viskores::FloatDefault spacing)
    : Spacing(spacing)
  {
  }

  template <typename T>
  VISKORES_EXEC void operator()(viskores::Id i, T& coord) const
  {
    coord = -10.0 + i * this->Spacing;
  }

private:
  viskores::FloatDefault Spacing;
};

struct ExplicitCoordsWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_CONT ExplicitCoordsWorklet(viskores::Vec3f spacings, viskores::Vec<viskores::Id, 3> dims)
    : Spacings(spacings)
    , Dims(dims)
  {
  }

  template <typename T>
  VISKORES_EXEC void operator()(viskores::Id pointId, viskores::Vec<T, 3>& coord) const
  {
    auto k = pointId % this->Dims[2];
    viskores::Id temp = pointId / this->Dims[2];
    auto j = temp % this->Dims[1];
    viskores::Id i = temp / this->Dims[1];
    coord =
      viskores::Vec3f(-10., -10., -10.) + this->Spacings * viskores::Vec<viskores::Id, 3>(i, j, k);
    if (this->Dims[2] == 1)
    {
      coord[2] = 0.0;
    }
  }

private:
  viskores::Vec3f Spacings;
  viskores::Vec<viskores::Id, 3> Dims;
};

struct TriangleIndicesWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayOut);
  using ExcecutionSignature = void(_1, _2);

  VISKORES_EXEC_CONT TriangleIndicesWorklet(viskores::Vec<viskores::Id, 2> dims)
    : Dims(dims)
  {
  }

  template <typename WritePortalType>
  VISKORES_EXEC void operator()(const viskores::Id quadId, const WritePortalType& quadAsTris) const
  {
    auto i = quadId % this->Dims[1];
    auto j = (quadId - i) / this->Dims[1];
    auto yoff = j * (this->Dims[0] + 1);
    // two tris per quad.
    quadAsTris.Set(quadId * 6 + 0, yoff + i);
    quadAsTris.Set(quadId * 6 + 1, yoff + i + (this->Dims[0] + 1));
    quadAsTris.Set(quadId * 6 + 2, yoff + i + 1 + (this->Dims[0] + 1));
    quadAsTris.Set(quadId * 6 + 3, yoff + i);
    quadAsTris.Set(quadId * 6 + 4, yoff + i + 1);
    quadAsTris.Set(quadId * 6 + 5, yoff + i + 1 + (this->Dims[0] + 1));
  }

private:
  viskores::Vec<viskores::Id, 2> Dims;
};

void CreateRectilinearMesh(unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ,
  conduit_cpp::Node& res, viskores::cont::ArrayHandleBasic<viskores::FloatDefault> outCoords[3],
  viskores::Int8 memorySpace)
{
  conduit_cpp::Node coords = res["coordsets/coords"];
  coords["type"] = "rectilinear";
  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  viskores::Vec3f spacings;

  spacings[0] = 20.0 / (nptsX - 1);
  spacings[1] = 20.0 / (nptsY - 1);
  spacings[2] = 0.0;

  if (nptsZ > 1)
  {
    spacings[2] = 20.0 / (nptsZ - 1);
  }
  viskores::Vec<viskores::Id, 3> dims({ nptsX, nptsY, nptsZ });
  for (int dim = 0; dim < 3; ++dim)
  {
    viskores::cont::Token token;
    outCoords[dim].PrepareForOutput(dims[dim], device, token);
  }
  conduit_cpp::Node coordVals = coords["values"];
  const char* axes[3] = { "x", "y", "z" };
  for (int dim = 0; dim < 3; ++dim)
  {
    if (dims[dim] > 1)
    {
      viskores::cont::Invoker invoke(device);
      RectilinearCoordsWorklet worker(spacings[dim]);
      invoke(worker, viskores::cont::make_ArrayHandleCounting(0, 1, dims[dim]), outCoords[dim]);
      if (auto ptr = outCoords[dim].GetReadPointer(device))
      {
        coordVals[axes[dim]].set_external(ptr, dims[dim]);
      }
    }
  }

  res["topologies/mesh/type"] = "rectilinear";
  res["topologies/mesh/coordset"] = "coords";
}

void CreateCoords(unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ,
  conduit_cpp::Node& res, viskores::cont::ArrayHandleSOA<viskores::Vec3f>& outCoords,
  viskores::Int8 memorySpace)
{
  conduit_cpp::Node coords = res["coordsets/coords"];
  conduit_cpp::Node coordVals = coords["values"];
  coords["type"] = "explicit";

  unsigned int npts = nptsX * nptsY;

  if (nptsZ > 1)
  {
    npts *= nptsZ;
  }
  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  {
    viskores::cont::Token token;
    outCoords.PrepareForOutput(npts, device, token);
  }
  viskores::Vec3f spacings;

  spacings[0] = 20.0 / (nptsX - 1);
  spacings[1] = 20.0 / (nptsY - 1);
  spacings[2] = 0.0;

  if (nptsZ > 1)
  {
    spacings[2] = 20.0 / (nptsZ - 1);
  }
  viskores::Vec<viskores::Id, 3> dims({ nptsX, nptsY, nptsZ });
  viskores::cont::Invoker invoke(device);
  ExplicitCoordsWorklet worker(spacings, dims);
  invoke(worker, viskores::cont::make_ArrayHandleCounting(0, 1, npts), outCoords);
  const char* axes[3] = { "x", "y", "z" };
  for (int dim = 0; dim < 3; ++dim)
  {
    if (auto ptr = outCoords.GetArray(dim).GetReadPointer(device))
    {
      coordVals[axes[dim]].set_external(ptr, npts);
    }
  }
}

void CreateStructuredMesh(unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ,
  conduit_cpp::Node& res, viskores::cont::ArrayHandleSOA<viskores::Vec3f>& outCoords,
  viskores::Int8 memorySpace)
{
  CreateCoords(nptsX, nptsY, nptsZ, res, outCoords, memorySpace);

  res["topologies/mesh/type"] = "structured";
  res["topologies/mesh/coordset"] = "coords";
  res["topologies/mesh/elements/dims/i"] = nptsX - 1;
  res["topologies/mesh/elements/dims/j"] = nptsY - 1;
  if (nptsZ > 0)
  {
    res["topologies/mesh/elements/dims/k"] = nptsZ - 1;
  }
}

void CreateTrisMesh(unsigned int nptsX, unsigned int nptsY, conduit_cpp::Node& res,
  viskores::cont::ArrayHandleSOA<viskores::Vec3f>& outCoords,
  viskores::cont::ArrayHandleBasic<unsigned int>& connectivity,
  viskores::cont::ArrayHandleBasic<viskores::Float64>& values, viskores::Int8 memorySpace)
{
  CreateStructuredMesh(nptsX, nptsY, 1, res, outCoords, memorySpace);

  unsigned int nElementX = nptsX - 1;
  unsigned int nElementY = nptsY - 1;
  unsigned int nElements = nElementX * nElementY;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";
  res["topologies/mesh/elements/shape"] = "tri";

  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  {
    viskores::cont::Token token;
    connectivity.PrepareForOutput(nElements * 6, device, token);
  }
  {
    viskores::cont::Invoker invoke(device);
    TriangleIndicesWorklet worker({ nElementX, nElementY });
    invoke(worker, viskores::cont::make_ArrayHandleCounting(0, 1, nElements), connectivity);
    if (auto ptr = connectivity.GetReadPointer(device))
    {
      res["topologies/mesh/elements/connectivity"].set_external(ptr, nElements * 6);
    }
  }

  // Need also to define 'fields' for cell array
  conduit_cpp::Node resFields = res["fields/field"];
  resFields["association"] = "element";
  resFields["topology"] = "mesh";
  resFields["volume_dependent"] = "false";

  viskores::Id numberofValues = nElements * 2;
  {
    viskores::cont::Token token;
    values.PrepareForOutput(numberofValues, device, token);
  }
  {
    ArrayCopy(viskores::cont::make_ArrayHandleCounting(0, 1, numberofValues), values);
    if (auto ptr = values.GetReadPointer(device))
    {
      resFields["values"].set_external(ptr, numberofValues);
    }
  }
}

inline unsigned int calc(unsigned int i, unsigned int j, unsigned int k, unsigned int I,
  unsigned int J, unsigned int K, unsigned int nx, unsigned int ny)
{
  return (i + I) + (j + J) * nx + (k + K) * (nx * ny);
}

void CreateMixedUnstructuredMesh(unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ,
  conduit_cpp::Node& res, viskores::cont::ArrayHandleSOA<viskores::Vec3f>& pointCoords,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemShapes,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemConnectivity,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemSizes,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemOffsets,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemShapes,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemConnectivity,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemSizes,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemOffsets, viskores::Int8 memorySpace)
{
  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  CreateCoords(nptsX, nptsY, nptsZ, res, pointCoords, memorySpace);

  res["state/time"] = 3.1415;
  res["state/cycle"] = 100UL;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";

  const unsigned int nElementX = nptsX - 1;
  const unsigned int nElementY = nptsY - 1;
  const unsigned int nElementZ = nptsZ - 1;

  const unsigned int nElementX2 = nElementX / 2;
  // one hexa divided into 3 tetras and one polyhedron (prism)
  const unsigned int nTet = 3 * nElementZ * nElementY * (nElementX2 + nElementX % 2);
  const unsigned int nPolyhedra = nElementZ * nElementY * (nElementX2 + nElementX % 2);
  // one hexa as hexahedron
  const unsigned int nHex = nElementZ * nElementY * nElementX2;

  const unsigned int nFaces = 5 * nPolyhedra;
  const unsigned int nEle = nTet + nHex + nPolyhedra;

  res["topologies/mesh/elements/shape"] = "mixed";
  // Viskores does not support VTK_POLYHEDRON
  // if we create the dataset in host mem. with device adapter serial
  // it will be read by VTK
  // when we create the datset in device mem. we a wedge instead
  if (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL)
  {
    res["topologies/mesh/elements/shape_map/polyhedral"] = VTK_POLYHEDRON;
  }
  else
  {
    res["topologies/mesh/elements/shape_map/wedge"] = VTK_WEDGE;
  }
  res["topologies/mesh/elements/shape_map/tet"] = VTK_TETRA;
  res["topologies/mesh/elements/shape_map/hex"] = VTK_HEXAHEDRON;

  res["topologies/mesh/subelements/shape"] = "mixed";
  res["topologies/mesh/subelements/shape_map/quad"] = VTK_QUAD;
  res["topologies/mesh/subelements/shape_map/tri"] = VTK_TRIANGLE;

  const auto elemConnectivitySize = nTet * 4 +
    // A wedge as a polyhedron (5 faces) for host memory
    // and as a cell (6 points) for device memory
    nPolyhedra * ((memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL) ? 5 : 6) + nHex * 8;
  const auto subElemConnectivitySize = nPolyhedra * 18;

  std::vector<unsigned int> elem_connectivity, elem_shapes, elem_sizes, elem_offsets;
  elem_shapes.resize(nEle);
  elem_sizes.resize(nEle);
  elem_offsets.resize(nEle);
  elem_connectivity.resize(elemConnectivitySize);
  elem_offsets[0] = 0;

  std::vector<unsigned int> subelem_connectivity, subelem_shapes, subelem_sizes, subelem_offsets;
  subelem_shapes.resize(nFaces);
  subelem_sizes.resize(nFaces);
  subelem_offsets.resize(nFaces);
  subelem_connectivity.resize(nPolyhedra * 18);
  subelem_offsets[0] = 0;

  unsigned int idx_elem = 0;
  unsigned int idx = 0;
  unsigned int idx_elem2 = 0;
  unsigned int idx2 = 0;
  unsigned int polyhedronCounter = 0;

  for (unsigned int k = 0; k < nElementZ; ++k)
  {
    for (unsigned int j = 0; j < nElementZ; ++j)
    {
      for (unsigned int i = 0; i < nElementX; ++i)
      {
        if (i % 2 == 1) // hexahedron
        {
          constexpr int HexaPointCount = 8;

          elem_shapes[idx_elem] = VTK_HEXAHEDRON;
          elem_sizes[idx_elem] = HexaPointCount;
          if (idx_elem + 1 < elem_offsets.size())
          {
            elem_offsets[idx_elem + 1] = elem_offsets[idx_elem] + HexaPointCount;
          }

          elem_connectivity[idx + 0] = calc(0, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 1] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 2] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 3] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 4] = calc(0, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 5] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 6] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 7] = calc(0, 1, 1, i, j, k, nptsX, nptsY);

          idx_elem += 1;
          idx += HexaPointCount;
        }
        else // 3 tets, one polyhedron for host memory (or wedge for device memory)
        {
          elem_shapes[idx_elem + 0] = VTK_TETRA;
          elem_shapes[idx_elem + 1] = VTK_TETRA;
          elem_shapes[idx_elem + 2] = VTK_TETRA;
          elem_shapes[idx_elem + 3] =
            (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL) ? VTK_POLYHEDRON : VTK_WEDGE;

          constexpr int TetraPointCount = 4;
          constexpr int WedgeFaceCount = 5;
          constexpr int WedgePointCount = 6;
          constexpr int TrianglePointCount = 3;
          constexpr int QuadPointCount = 4;

          elem_sizes[idx_elem + 0] = TetraPointCount;
          elem_sizes[idx_elem + 1] = TetraPointCount;
          elem_sizes[idx_elem + 2] = TetraPointCount;
          elem_sizes[idx_elem + 3] =
            (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL) ? WedgeFaceCount : WedgePointCount;

          elem_offsets[idx_elem + 1] = elem_offsets[idx_elem + 0] + TetraPointCount;
          elem_offsets[idx_elem + 2] = elem_offsets[idx_elem + 1] + TetraPointCount;
          elem_offsets[idx_elem + 3] = elem_offsets[idx_elem + 2] + TetraPointCount;
          if (idx_elem + 4 < elem_offsets.size())
          {
            elem_offsets[idx_elem + 4] = elem_offsets[idx_elem + 3] +
              ((memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL) ? WedgeFaceCount : WedgePointCount);
          }

          elem_connectivity[idx + 0] = calc(0, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 1] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 2] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 3] = calc(0, 0, 1, i, j, k, nptsX, nptsY);

          elem_connectivity[idx + 4] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 5] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 6] = calc(0, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 7] = calc(0, 1, 1, i, j, k, nptsX, nptsY);

          elem_connectivity[idx + 8] = calc(0, 0, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 9] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 10] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          elem_connectivity[idx + 11] = calc(1, 0, 0, i, j, k, nptsX, nptsY);

          // Viskores does not support polyhedra or storing faces
          // host memory datasets are processed by VTK
          if (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL)
          {
            // note: there are no shared faces in this example
            elem_connectivity[idx + 12] = 0 + WedgeFaceCount * polyhedronCounter;
            elem_connectivity[idx + 13] = 1 + WedgeFaceCount * polyhedronCounter;
            elem_connectivity[idx + 14] = 2 + WedgeFaceCount * polyhedronCounter;
            elem_connectivity[idx + 15] = 3 + WedgeFaceCount * polyhedronCounter;
            elem_connectivity[idx + 16] = 4 + WedgeFaceCount * polyhedronCounter;

            subelem_shapes[idx_elem2 + 0] = VTK_QUAD;
            subelem_shapes[idx_elem2 + 1] = VTK_QUAD;
            subelem_shapes[idx_elem2 + 2] = VTK_QUAD;
            subelem_shapes[idx_elem2 + 3] = VTK_TRIANGLE;
            subelem_shapes[idx_elem2 + 4] = VTK_TRIANGLE;

            subelem_sizes[idx_elem2 + 0] = QuadPointCount;
            subelem_sizes[idx_elem2 + 1] = QuadPointCount;
            subelem_sizes[idx_elem2 + 2] = QuadPointCount;
            subelem_sizes[idx_elem2 + 3] = TrianglePointCount;
            subelem_sizes[idx_elem2 + 4] = TrianglePointCount;

            subelem_offsets[idx_elem2 + 1] = subelem_offsets[idx_elem2 + 0] + QuadPointCount;
            subelem_offsets[idx_elem2 + 2] = subelem_offsets[idx_elem2 + 1] + QuadPointCount;
            subelem_offsets[idx_elem2 + 3] = subelem_offsets[idx_elem2 + 2] + QuadPointCount;
            subelem_offsets[idx_elem2 + 4] = subelem_offsets[idx_elem2 + 3] + TrianglePointCount;
            if (idx_elem2 + 5 < subelem_offsets.size())
            {
              subelem_offsets[idx_elem2 + 5] = subelem_offsets[idx_elem2 + 4] + TrianglePointCount;
            }

            subelem_connectivity[idx2 + 0] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 1] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 2] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 3] = calc(0, 1, 0, i, j, k, nptsX, nptsY);

            subelem_connectivity[idx2 + 4] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 5] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 6] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 7] = calc(1, 0, 1, i, j, k, nptsX, nptsY);

            subelem_connectivity[idx2 + 8] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 9] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 10] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 11] = calc(1, 1, 1, i, j, k, nptsX, nptsY);

            subelem_connectivity[idx2 + 12] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 13] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 14] = calc(1, 1, 0, i, j, k, nptsX, nptsY);

            subelem_connectivity[idx2 + 15] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 16] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
            subelem_connectivity[idx2 + 17] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
          }
          else
          {
            elem_connectivity[idx + 12] = calc(1, 0, 1, i, j, k, nptsX, nptsY);
            elem_connectivity[idx + 13] = calc(1, 1, 1, i, j, k, nptsX, nptsY);
            elem_connectivity[idx + 14] = calc(0, 1, 1, i, j, k, nptsX, nptsY);
            elem_connectivity[idx + 15] = calc(1, 0, 0, i, j, k, nptsX, nptsY);
            elem_connectivity[idx + 16] = calc(1, 1, 0, i, j, k, nptsX, nptsY);
            elem_connectivity[idx + 17] = calc(0, 1, 0, i, j, k, nptsX, nptsY);
          }

          idx_elem += 4; // three tets, 1 polyhedron
          idx += 3 * TetraPointCount +
            ((memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL) ? WedgeFaceCount : WedgePointCount);
          polyhedronCounter += 1;
          // these are only used for subelem, so we don't need to branch
          // on polyhedron
          idx_elem2 += WedgeFaceCount; // five faces on the polyhedron
          idx2 += 3 * QuadPointCount + 2 * TrianglePointCount;
        }
      }
    }
  }

  ArrayCopy(viskores::cont::make_ArrayHandle(elem_offsets, viskores::CopyFlag::Off), elemOffsets);
  ArrayCopy(viskores::cont::make_ArrayHandle(elem_sizes, viskores::CopyFlag::Off), elemSizes);
  ArrayCopy(viskores::cont::make_ArrayHandle(elem_shapes, viskores::CopyFlag::Off), elemShapes);
  ArrayCopy(
    viskores::cont::make_ArrayHandle(elem_connectivity, viskores::CopyFlag::Off), elemConnectivity);

  if (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL)
  {
    ArrayCopy(
      viskores::cont::make_ArrayHandle(subelem_offsets, viskores::CopyFlag::Off), subelemOffsets);
    ArrayCopy(
      viskores::cont::make_ArrayHandle(subelem_sizes, viskores::CopyFlag::Off), subelemSizes);
    ArrayCopy(
      viskores::cont::make_ArrayHandle(subelem_shapes, viskores::CopyFlag::Off), subelemShapes);
    ArrayCopy(viskores::cont::make_ArrayHandle(subelem_connectivity, viskores::CopyFlag::Off),
      subelemConnectivity);
  }

  auto elements = res["topologies/mesh/elements"];
  elements["shapes"].set_external(elemShapes.GetReadPointer(device), nEle);
  elements["offsets"].set_external(elemOffsets.GetReadPointer(device), nEle);
  elements["sizes"].set_external(elemSizes.GetReadPointer(device), nEle);
  elements["connectivity"].set_external(
    elemConnectivity.GetReadPointer(device), elemConnectivitySize);

  if (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL)
  {
    auto subelements = res["topologies/mesh/subelements"];
    subelements["shapes"].set_external(subelemShapes.GetReadPointer(device), nFaces);
    subelements["offsets"].set_external(subelemOffsets.GetReadPointer(device), nFaces);
    subelements["sizes"].set_external(subelemSizes.GetReadPointer(device), nFaces);
    subelements["connectivity"].set_external(
      subelemConnectivity.GetReadPointer(device), subElemConnectivitySize);
  }
}

void CreateMixedUnstructuredMesh2D(unsigned int npts_x, unsigned int npts_y, conduit_cpp::Node& res,
  viskores::cont::ArrayHandleSOA<viskores::Vec3f>& pointCoords,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemShapes,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemConnectivity,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemSizes,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemOffsets, viskores::Int8 memorySpace)
{
  CreateCoords(npts_x, npts_y, 1, res, pointCoords, memorySpace);

  const unsigned int nele_x = npts_x - 1;
  const unsigned int nele_y = npts_y - 1;

  res["state/time"] = 3.1415;
  res["state/cycle"] = 100UL;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";

  res["topologies/mesh/elements/shape"] = "mixed";
  res["topologies/mesh/elements/shape_map/quad"] = VTK_QUAD;
  res["topologies/mesh/elements/shape_map/tri"] = VTK_TRIANGLE;

  const unsigned int nele_x2 = nele_x / 2;
  const unsigned int nquads = nele_y * nele_x2;
  const unsigned int ntris = nele_y * 2 * (nele_x2 + nele_x % 2);
  const unsigned int nele = nquads + ntris;

  std::vector<unsigned int> connectivity, sizes, offsets, shapes;
  shapes.resize(nele);
  sizes.resize(nele);
  offsets.resize(nele);
  offsets[0] = 0;
  connectivity.resize(nquads * 4 + ntris * 3);

  size_t idx_elem = 0;
  size_t idx = 0;

  for (unsigned int j = 0; j < nele_y; ++j)
  {
    for (unsigned int i = 0; i < nele_x; ++i)
    {
      if (i % 2 == 0)
      {
        constexpr int TrianglePointCount = 3;
        shapes[idx_elem + 0] = VTK_TRIANGLE;
        shapes[idx_elem + 1] = VTK_TRIANGLE;
        sizes[idx_elem + 0] = 3;
        sizes[idx_elem + 1] = 3;

        offsets[idx_elem + 1] = offsets[idx_elem + 0] + TrianglePointCount;
        if (idx_elem + 2 < offsets.size())
        {
          offsets[idx_elem + 2] = offsets[idx_elem + 1] + TrianglePointCount;
        }

        connectivity[idx + 0] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 1] = calc(1, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 2] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);

        connectivity[idx + 3] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 4] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 5] = calc(0, 1, 0, i, j, 0, npts_x, npts_y);

        idx_elem += 2;
        idx += 6;
      }
      else
      {
        constexpr int QuadPointCount = 4;
        shapes[idx_elem] = VTK_QUAD;

        sizes[idx_elem] = 4;
        if (idx_elem + 1 < offsets.size())
        {
          offsets[idx_elem + 1] = offsets[idx_elem + 0] + QuadPointCount;
        }

        connectivity[idx + 0] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 1] = calc(1, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 2] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 3] = calc(0, 1, 0, i, j, 0, npts_x, npts_y);

        idx_elem += 1;
        idx += 4;
      }
    }
  }

  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);

  ArrayCopy(viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::Off), elemOffsets);
  ArrayCopy(viskores::cont::make_ArrayHandle(sizes, viskores::CopyFlag::Off), elemSizes);
  ArrayCopy(viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::Off), elemShapes);
  ArrayCopy(
    viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::Off), elemConnectivity);

  auto elements = res["topologies/mesh/elements"];
  elements["shapes"].set_external(elemShapes.GetReadPointer(device), nele);
  elements["offsets"].set_external(elemOffsets.GetReadPointer(device), nele);
  elements["sizes"].set_external(elemSizes.GetReadPointer(device), nele);
  elements["connectivity"].set_external(
    elemConnectivity.GetReadPointer(device), nquads * 4 + ntris * 3);
}

bool ValidateMeshTypeRectilinearImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault> pointCoords[3];
  CreateRectilinearMesh(3, 3, 3, mesh, pointCoords, memorySpace);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "missing partition 0");
  int dims[3];
  rg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 3, "incorrect y dimension expected=3, got=%d", dims[1]);
  VERIFY(dims[2] == 3, "incorrect z dimension expected=3, got=%d", dims[2]);
  std::array<double, 6> bounds;
  rg->GetBounds(bounds.data());
  VERIFY(
    bounds[0] == -10.0, "incorrect lower bound for x dimension expected=-10.0, got=%f", bounds[0])
  VERIFY(
    bounds[1] == 10.0, "incorrect upper bound for x dimension expected=10.0, got=%f", bounds[1])
  VERIFY(
    bounds[2] == -10.0, "incorrect lower bound for y dimension expected=-10.0, got=%f", bounds[2])
  VERIFY(
    bounds[3] == 10.0, "incorrect upper bound for y dimension expected=10.0, got=%f", bounds[3])
  VERIFY(
    bounds[4] == -10.0, "incorrect lower bound for z dimension expected=-10.0, got=%f", bounds[4])
  VERIFY(
    bounds[5] == 10.0, "incorrect upper bound for z dimension expected=10.0, got=%f", bounds[5])
  return true;
}

bool ValidateMeshTypeStructuredImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  viskores::cont::ArrayHandleSOA<viskores::Vec3f> pointCoords;
  CreateStructuredMesh(3, 3, 3, mesh, pointCoords, memorySpace);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto sg = vtkStructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(sg != nullptr, "missing partition 0");
  int dims[3];
  sg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 3, "incorrect y dimension expected=3, got=%d", dims[1]);
  VERIFY(dims[2] == 3, "incorrect z dimension expected=3, got=%d", dims[2]);
  std::array<double, 6> bounds;
  sg->GetBounds(bounds.data());
  VERIFY(
    bounds[0] == -10.0, "incorrect lower bound for x dimension expected=-10.0, got=%f", bounds[0])
  VERIFY(
    bounds[1] == 10.0, "incorrect upper bound for x dimension expected=10.0, got=%f", bounds[1])
  VERIFY(
    bounds[2] == -10.0, "incorrect lower bound for y dimension expected=-10.0, got=%f", bounds[2])
  VERIFY(
    bounds[3] == 10.0, "incorrect upper bound for y dimension expected=10.0, got=%f", bounds[3])
  VERIFY(
    bounds[4] == -10.0, "incorrect lower bound for z dimension expected=-10.0, got=%f", bounds[4])
  VERIFY(
    bounds[5] == 10.0, "incorrect upper bound for z dimension expected=10.0, got=%f", bounds[5])
  return true;
}

bool ValidateMeshTypeUnstructuredImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  // generate simple explicit tri-based 2d 'basic' mesh
  viskores::cont::ArrayHandleSOA<viskores::Vec3f> pointCoords;
  viskores::cont::ArrayHandleBasic<unsigned int> connectivity;
  viskores::cont::ArrayHandleBasic<viskores::Float64> values;
  CreateTrisMesh(3, 3, mesh, pointCoords, connectivity, values, memorySpace);

  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(ug != nullptr, "missing partition 0");

  VERIFY(ug->GetNumberOfPoints() == 9,
    "incorrect number of points, expected 9, got %" VTK_ID_TYPE_PRId, ug->GetNumberOfPoints());
  VERIFY(ug->GetNumberOfCells() == 8,
    "incorrect number of cells, expected 8, got %" VTK_ID_TYPE_PRId, ug->GetNumberOfCells());
  VERIFY(ug->GetCellData()->GetArray("field") != nullptr, "missing 'field' cell-data array");
  std::array<double, 6> bounds;
  ug->GetBounds(bounds.data());
  VERIFY(
    bounds[0] == -10.0, "incorrect lower bound for x dimension expected=-10.0, got=%f", bounds[0])
  VERIFY(
    bounds[1] == 10.0, "incorrect upper bound for x dimension expected=10.0, got=%f", bounds[1])
  VERIFY(
    bounds[2] == -10.0, "incorrect lower bound for y dimension expected=-10.0, got=%f", bounds[2])
  VERIFY(
    bounds[3] == 10.0, "incorrect upper bound for y dimension expected=10.0, got=%f", bounds[3])
  VERIFY(bounds[4] == 0.0, "incorrect lower bound for z dimension expected=0.0, got=%f", bounds[4])
  VERIFY(bounds[5] == 0.0, "incorrect upper bound for z dimension expected=0.0, got=%f", bounds[5])
  return true;
}

bool ValidateRectilinearGridWithDifferentDimensionsImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault> pointCoords[3];
  CreateRectilinearMesh(3, 2, 1, mesh, pointCoords, memorySpace);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "invalid partition at index 0");
  int dims[3];
  rg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 2, "incorrect y dimension expected=2, got=%d", dims[1]);
  VERIFY(dims[2] == 1, "incorrect z dimension expected=1, got=%d", dims[2]);

  return true;
}

bool Validate1DRectilinearGridImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  auto xAH = viskores::cont::make_ArrayHandle({ 5.0, 6.0, 7.0 });
  auto fieldAH = viskores::cont::make_ArrayHandle({ 0.0, 1.0 });

  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  conduit_cpp::Node mesh;
  auto coords = mesh["coordsets/coords"];
  coords["type"] = "rectilinear";
  coords["values/x"].set_external(xAH.GetReadPointer(device), 3);
  auto topo_mesh = mesh["topologies/mesh"];
  topo_mesh["type"] = "rectilinear";
  topo_mesh["coordset"] = "coords";
  auto field = mesh["fields/field"];
  field["association"] = "element";
  field["topology"] = "mesh";
  field["volume_dependent"] = "false";
  field["values"].set_external(fieldAH.GetReadPointer(device), 2);

  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "invalid partition at index 0");
  int dims[3];
  rg->GetDimensions(dims);
  VERIFY(dims[0] == 3, "incorrect x dimension expected=3, got=%d", dims[0]);
  VERIFY(dims[1] == 1, "incorrect y dimension expected=1, got=%d", dims[1]);
  VERIFY(dims[2] == 1, "incorrect z dimension expected=1, got=%d", dims[2]);

  return true;
}

bool ValidateMeshTypeMixedImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  constexpr int nX = 5, nY = 5, nZ = 5;
  viskores::cont::ArrayHandleSOA<viskores::Vec3f> pointCoords;
  viskores::cont::ArrayHandleBasic<unsigned int> elem_connectivity, elem_sizes, elem_offsets;
  viskores::cont::ArrayHandleBasic<unsigned int> subelem_connectivity, subelem_sizes,
    subelem_offsets;
  viskores::cont::ArrayHandleBasic<unsigned int> elem_shapes, subelem_shapes;
  CreateMixedUnstructuredMesh(5, 5, 5, mesh, pointCoords, elem_shapes, elem_connectivity,
    elem_sizes, elem_offsets, subelem_shapes, subelem_connectivity, subelem_sizes, subelem_offsets,
    memorySpace);
  const auto data = Convert(mesh);

  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  const auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));

  VERIFY(ug->GetNumberOfPoints() == nX * nY * nZ, "expected %d points got %" VTK_ID_TYPE_PRId,
    nX * nY * nZ, ug->GetNumberOfPoints());

  // 160 cells expected: 4 layers of
  //                     - 2 columns with 4 hexahedra
  //                     - 2 columns with 4 polyhedra (wedges) and 12 tetra
  //                     96 tetras + 32 hexas + 32 polyhedra
  VERIFY(ug->GetNumberOfCells() == 160, "expected 160 cells, got %" VTK_ID_TYPE_PRId,
    ug->GetNumberOfCells());

  // check cell types
  const auto it = vtkSmartPointer<vtkCellIterator>::Take(ug->NewCellIterator());

  int nPolyhedra(0), nTetra(0), nHexa(0), nCells(0);
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    ++nCells;
    const int cellType = it->GetCellType();
    switch (cellType)
    {
      case VTK_POLYHEDRON:
      {
        if (memorySpace == VISKORES_DEVICE_ADAPTER_SERIAL)
        {
          ++nPolyhedra;
          const vtkIdType nFaces = it->GetNumberOfFaces();
          VERIFY(nFaces == 5, "Expected 5 faces, got %" VTK_ID_TYPE_PRId, nFaces);
          break;
        }
        else
        {
          vtkLog(ERROR, "Expected only tetras, hexas and wedges.");
          return false;
        }
      }
      case VTK_WEDGE:
      {
        if (memorySpace != VISKORES_DEVICE_ADAPTER_SERIAL)
        {
          // this is a wedge for device memory as Viskores does not have polyhedra
          ++nPolyhedra;
          break;
        }
        else
        {
          vtkLog(ERROR, "Expected only tetras, hexas and polyhedra.");
          return false;
        }
      }
      case VTK_HEXAHEDRON:
      {
        ++nHexa;
        break;
      }
      case VTK_TETRA:
      {
        ++nTetra;
        break;
      }
      default:
      {
        vtkLog(ERROR, "Expected only tetras, hexas and polyhedra.");
        return false;
      }
    }
  }

  VERIFY(nCells == 160, "Expected 160 cells, got %d", nCells);
  VERIFY(nTetra == 96, "Expected 96 tetras, got %d", nTetra);
  VERIFY(nHexa == 32, "Expected 32 hexahedra, got %d", nHexa);
  VERIFY(nPolyhedra == 32, "Expected 32 polyhedra, got %d", nPolyhedra);
  std::array<double, 6> bounds;
  ug->GetBounds(bounds.data());
  VERIFY(
    bounds[0] == -10.0, "incorrect lower bound for x dimension expected=-10.0, got=%f", bounds[0])
  VERIFY(
    bounds[1] == 10.0, "incorrect upper bound for x dimension expected=10.0, got=%f", bounds[1])
  VERIFY(
    bounds[2] == -10.0, "incorrect lower bound for y dimension expected=-10.0, got=%f", bounds[2])
  VERIFY(
    bounds[3] == 10.0, "incorrect upper bound for y dimension expected=10.0, got=%f", bounds[3])
  VERIFY(
    bounds[4] == -10.0, "incorrect lower bound for z dimension expected=-10.0, got=%f", bounds[4])
  VERIFY(
    bounds[5] == 10.0, "incorrect upper bound for z dimension expected=10.0, got=%f", bounds[5])

  return true;
}

bool ValidateMeshTypeMixed2DImpl(viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  viskores::cont::ArrayHandleSOA<viskores::Vec3f> pointCoords;
  viskores::cont::ArrayHandleBasic<unsigned int> elem_connectivity, elem_sizes, elem_offsets;
  viskores::cont::ArrayHandleBasic<unsigned int> elem_shapes;
  CreateMixedUnstructuredMesh2D(
    5, 5, mesh, pointCoords, elem_shapes, elem_connectivity, elem_sizes, elem_offsets, memorySpace);
  const auto data = Convert(mesh);

  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));

  // 16 triangles, 4 quads: 24 cells
  VERIFY(ug->GetNumberOfCells() == 24, "expected 24 cells, got %" VTK_ID_TYPE_PRId,
    ug->GetNumberOfCells());
  VERIFY(ug->GetNumberOfPoints() == 25, "Expected 25 points, got %" VTK_ID_TYPE_PRId,
    ug->GetNumberOfPoints());

  // check cell types
  const auto it = vtkSmartPointer<vtkCellIterator>::Take(ug->NewCellIterator());
  int nTris(0), nQuads(0);
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    const int cellType = it->GetCellType();
    switch (cellType)
    {
      case VTK_TRIANGLE:
      {
        ++nTris;
        break;
      }
      case VTK_QUAD:
      {
        ++nQuads;
        break;
      }
      default:
      {
        vtkLog(ERROR, "Expected only triangles and quads.");
        return false;
      }
    }
  }

  std::array<double, 6> bounds;
  ug->GetBounds(bounds.data());
  VERIFY(
    bounds[0] == -10.0, "incorrect lower bound for x dimension expected=-10.0, got=%f", bounds[0])
  VERIFY(
    bounds[1] == 10.0, "incorrect upper bound for x dimension expected=10.0, got=%f", bounds[1])
  VERIFY(
    bounds[2] == -10.0, "incorrect lower bound for y dimension expected=-10.0, got=%f", bounds[2])
  VERIFY(
    bounds[3] == 10.0, "incorrect upper bound for y dimension expected=10.0, got=%f", bounds[3])
  VERIFY(bounds[4] == 0.0, "incorrect lower bound for z dimension expected=0.0, got=%f", bounds[4])
  VERIFY(bounds[5] == 0.0, "incorrect upper bound for z dimension expected=0.0, got=%f", bounds[5])
  return true;
}

bool ValidateMeshTypeAMRImpl(const std::string& file, viskores::Int8 memorySpace)
{
  SCOPED_RUNTIME_DEVICE_SELECTOR(memorySpace);
  conduit_cpp::Node mesh;
  // read in an example mesh dataset
  conduit_node_load(conduit_cpp::c_node(&mesh), file.c_str(), "");

  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  // add in point data
  std::string field_name = "pointfield";
  double field_value = 1;
  size_t num_children = mesh["data"].number_of_children();
  std::vector<viskores::cont::ArrayHandle<viskores::Float64>>
    pointValuesAHs; // keeps device data alive.
  for (size_t i = 0; i < num_children; i++)
  {
    conduit_cpp::Node amr_block = mesh["data"].child(i);
    int i_dimension = amr_block["coordsets/coords/dims/i"].to_int32();
    int j_dimension = amr_block["coordsets/coords/dims/j"].to_int32();
    int k_dimension = amr_block["coordsets/coords/dims/k"].to_int32();
    conduit_cpp::Node fields = amr_block["fields"];
    conduit_cpp::Node point_field = fields[field_name];
    point_field["association"] = "vertex";
    point_field["topology"] = "topo";
    viskores::cont::ArrayHandleBasic<viskores::Float64> ah;
    {
      viskores::cont::Token token;
      ah.PrepareForOutput((i_dimension + 1) * (j_dimension + 1) * (k_dimension + 1), device, token);
    }
    ah.Fill(field_value);
    auto pointFieldValues = point_field["values"];
    pointFieldValues.set_external(ah.GetReadPointer(device), ah.GetNumberOfValues());
    pointValuesAHs.emplace_back(ah);
  }

  const auto& meshdata = mesh["data"];
  // run vtk conduit source
  vtkNew<vtkConduitSource> source;
  source->SetUseAMRMeshProtocol(true);
  source->SetNode(conduit_cpp::c_node(&meshdata));
  source->Update();
  auto data = source->GetOutputDataObject(0);

  VERIFY(vtkOverlappingAMR::SafeDownCast(data) != nullptr,
    "Incorrect data type, expected vtkOverlappingAMR, got %s", vtkLogIdentifier(data));

  auto amr = vtkOverlappingAMR::SafeDownCast(data);

  std::vector<double> bounds(6);
  std::vector<double> origin(3);

  amr->GetBounds(bounds.data());
  amr->GetOrigin(0, 0, origin.data());

  VERIFY(bounds[0] == 0 && bounds[1] == 1 && bounds[2] == 0 && bounds[3] == 1 && bounds[4] == 0 &&
      bounds[5] == 1,
    "Incorrect AMR bounds");

  VERIFY(origin[0] == 0 && origin[1] == 0 && origin[2] == 0, "Incorrect AMR origin");

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(amr->NewIterator());
  iter->InitTraversal();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* block = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    VERIFY(block->GetCellData()->GetArray("density") != nullptr, "Incorrect AMR cell data");
    double range[2] = { -1, -1 };
    block->GetPointData()->GetArray(field_name.c_str())->GetRange(range);
    VERIFY(range[0] == field_value && range[1] == field_value, "Incorrect AMR point data");
  }

  return true;
}

bool ValidateMeshTypeStructured()
{
  try
  {
    VERIFY(ValidateMeshTypeStructuredImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeStructuredImpl with serial device failed.");
    VERIFY(ValidateMeshTypeStructuredImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateMeshTypeStructuredImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool ValidateMeshTypeRectilinear()
{
  try
  {
    VERIFY(ValidateMeshTypeRectilinearImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeRectilinearImpl with serial device failed.");
    VERIFY(ValidateMeshTypeRectilinearImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateMeshTypeRectilinearImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool ValidateMeshTypeUnstructured()
{
  try
  {
    VERIFY(ValidateMeshTypeUnstructuredImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeUnstructuredImpl with serial device failed.");
    VERIFY(ValidateMeshTypeUnstructuredImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateMeshTypeUnstructuredImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool ValidateRectilinearGridWithDifferentDimensions()
{
  try
  {
    VERIFY(ValidateRectilinearGridWithDifferentDimensionsImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateRectilinearGridWithDifferentDimensionsImpl with serial device failed.");
    VERIFY(ValidateRectilinearGridWithDifferentDimensionsImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateRectilinearGridWithDifferentDimensionsImpl with CUDA device FAILED.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool Validate1DRectilinearGrid()
{
  try
  {
    VERIFY(Validate1DRectilinearGridImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "Validate1DRectilinearGridImpl with serial device failed.");
    VERIFY(Validate1DRectilinearGridImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "Validate1DRectilinearGridImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool ValidateMeshTypeMixed()
{
  try
  {
    VERIFY(ValidateMeshTypeMixedImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeMixedImpl with serial device failed.");
    VERIFY(ValidateMeshTypeMixedImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateMeshTypeMixedImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool ValidateMeshTypeMixed2D()
{
  try
  {
    VERIFY(ValidateMeshTypeMixed2DImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeMixed2DImpl with serial device failed.");
    VERIFY(ValidateMeshTypeMixed2DImpl(VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateMeshTypeMixed2DImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

bool ValidateMeshTypeAMR(const std::string& file)
{
  try
  {
    VERIFY(ValidateMeshTypeAMRImpl(file, VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeAMRImpl with serial device failed.");
    VERIFY(ValidateMeshTypeAMRImpl(file, VISKORES_DEVICE_ADAPTER_CUDA),
      "ValidateMeshTypeAMRImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

void CreatePolyhedra(Grid& grid, Attributes& attribs, unsigned int nx, unsigned int ny,
  unsigned int nz, conduit_cpp::Node& mesh, viskores::Int8 memorySpace,
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault>& points,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemConnectivity,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemSizes,
  viskores::cont::ArrayHandleBasic<unsigned int>& elemOffsets,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemConnectivity,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemSizes,
  viskores::cont::ArrayHandleBasic<unsigned int>& subelemOffsets,
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault>& velocity,
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault>& pressure)
{
  auto device = viskores::cont::make_DeviceAdapterId(memorySpace);
  unsigned int numPoints[3] = { nx, ny, nz };
  double spacing[3] = { 1, 1.1, 1.3 };
  grid.Initialize(numPoints, spacing);
  attribs.Initialize(&grid);
  attribs.UpdateFields(0);

  ArrayCopy(viskores::cont::make_ArrayHandle(grid.GetPoints(), viskores::CopyFlag::Off), points);
  mesh["coordsets/coords/type"].set("explicit");
  mesh["coordsets/coords/values/x"].set_external(points.GetReadPointer(device),
    grid.GetNumberOfPoints(), /*offset=*/0, /*stride=*/3 * sizeof(viskores::FloatDefault));
  mesh["coordsets/coords/values/y"].set_external(points.GetReadPointer(device),
    grid.GetNumberOfPoints(),
    /*offset=*/sizeof(viskores::FloatDefault), /*stride=*/3 * sizeof(viskores::FloatDefault));
  mesh["coordsets/coords/values/z"].set_external(points.GetReadPointer(device),
    grid.GetNumberOfPoints(),
    /*offset=*/2 * sizeof(viskores::FloatDefault), /*stride=*/3 * sizeof(viskores::FloatDefault));

  // Next, add topology
  mesh["topologies/mesh/type"].set("unstructured");
  mesh["topologies/mesh/coordset"].set("coords");

  // add elements
  ArrayCopy(viskores::cont::make_ArrayHandle(
              grid.GetPolyhedralCells().Connectivity, viskores::CopyFlag::Off),
    elemConnectivity);
  ArrayCopy(
    viskores::cont::make_ArrayHandle(grid.GetPolyhedralCells().Sizes, viskores::CopyFlag::Off),
    elemSizes);
  ArrayCopy(
    viskores::cont::make_ArrayHandle(grid.GetPolyhedralCells().Offsets, viskores::CopyFlag::Off),
    elemOffsets);
  mesh["topologies/mesh/elements/shape"].set("polyhedral");
  mesh["topologies/mesh/elements/connectivity"].set_external(
    elemConnectivity.GetReadPointer(device), grid.GetPolyhedralCells().Connectivity.size());
  mesh["topologies/mesh/elements/sizes"].set_external(
    elemSizes.GetReadPointer(device), grid.GetPolyhedralCells().Sizes.size());
  mesh["topologies/mesh/elements/offsets"].set_external(
    elemOffsets.GetReadPointer(device), grid.GetPolyhedralCells().Offsets.size());

  // add faces (aka subelements)
  ArrayCopy(viskores::cont::make_ArrayHandle(
              grid.GetPolygonalFaces().Connectivity, viskores::CopyFlag::Off),
    subelemConnectivity);
  ArrayCopy(
    viskores::cont::make_ArrayHandle(grid.GetPolygonalFaces().Sizes, viskores::CopyFlag::Off),
    subelemSizes);
  ArrayCopy(
    viskores::cont::make_ArrayHandle(grid.GetPolygonalFaces().Offsets, viskores::CopyFlag::Off),
    subelemOffsets);
  mesh["topologies/mesh/subelements/shape"].set("polygonal");
  mesh["topologies/mesh/subelements/connectivity"].set_external(
    subelemConnectivity.GetReadPointer(device), grid.GetPolygonalFaces().Connectivity.size());
  mesh["topologies/mesh/subelements/sizes"].set_external(
    subelemSizes.GetReadPointer(device), grid.GetPolygonalFaces().Sizes.size());
  mesh["topologies/mesh/subelements/offsets"].set_external(
    subelemOffsets.GetReadPointer(device), grid.GetPolygonalFaces().Offsets.size());

  // Finally, add fields.
  ArrayCopy(viskores::cont::make_ArrayHandle(attribs.GetVelocityArray(), viskores::CopyFlag::Off),
    velocity);
  ArrayCopy(viskores::cont::make_ArrayHandle(attribs.GetPressureArray(), viskores::CopyFlag::Off),
    pressure);
  auto fields = mesh["fields"];
  fields["velocity/association"].set("vertex");
  fields["velocity/topology"].set("mesh");
  fields["velocity/volume_dependent"].set("false");

  // velocity is stored in non-interlaced form (unlike points).
  fields["velocity/values/x"].set_external(velocity.GetReadPointer(device),
    grid.GetNumberOfPoints(),
    /*offset=*/0);
  fields["velocity/values/y"].set_external(velocity.GetReadPointer(device),
    grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(viskores::FloatDefault));
  fields["velocity/values/z"].set_external(velocity.GetReadPointer(device),
    grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(viskores::FloatDefault) * 2);

  // pressure is cell-data.
  fields["pressure/association"].set("element");
  fields["pressure/topology"].set("mesh");
  fields["pressure/volume_dependent"].set("false");
  fields["pressure/values"].set_external(pressure.GetReadPointer(device), grid.GetNumberOfCells());
}

bool ValidatePolyhedraImpl(viskores::Int8 memorySpace)
{
  conduit_cpp::Node mesh;
  constexpr int nX = 4, nY = 4, nZ = 4;
  Grid grid;
  Attributes attribs;
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault> points;
  viskores::cont::ArrayHandleBasic<unsigned int> elemConnectivity;
  viskores::cont::ArrayHandleBasic<unsigned int> elemSizes;
  viskores::cont::ArrayHandleBasic<unsigned int> elemOffsets;
  viskores::cont::ArrayHandleBasic<unsigned int> subelemConnectivity;
  viskores::cont::ArrayHandleBasic<unsigned int> subelemSizes;
  viskores::cont::ArrayHandleBasic<unsigned int> subelemOffsets;
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault> velocity;
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault> pressure;
  CreatePolyhedra(grid, attribs, nX, nY, nZ, mesh, memorySpace, points, elemConnectivity, elemSizes,
    elemOffsets, subelemConnectivity, subelemSizes, subelemOffsets, velocity, pressure);
  auto values = mesh["fields/velocity/values"];
  auto data = Convert(mesh);

  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));

  VERIFY(ug->GetNumberOfPoints() == static_cast<vtkIdType>(grid.GetNumberOfPoints()),
    "expected %zu points got %" VTK_ID_TYPE_PRId, grid.GetNumberOfPoints(),
    ug->GetNumberOfPoints());

  VERIFY(ug->GetNumberOfCells() == static_cast<vtkIdType>(grid.GetNumberOfCells()),
    "expected %zu cells, got %" VTK_ID_TYPE_PRId, grid.GetNumberOfCells(), ug->GetNumberOfCells());

  // check cell types
  auto it = vtkSmartPointer<vtkCellIterator>::Take(ug->NewCellIterator());

  vtkIdType nPolyhedra(0);
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    const int cellType = it->GetCellType();
    switch (cellType)
    {
      case VTK_POLYHEDRON:
      {
        ++nPolyhedra;
        const vtkIdType nFaces = it->GetNumberOfFaces();
        VERIFY(nFaces == 6, "Expected 6 faces, got %" VTK_ID_TYPE_PRId, nFaces);
        break;
      }
      default:
      {
        vtkLog(ERROR, "Expected only polyhedra.");
        return false;
      }
    }
  }

  VERIFY(nPolyhedra == static_cast<vtkIdType>(grid.GetNumberOfCells()),
    "Expected %zu polyhedra, got %" VTK_ID_TYPE_PRId, grid.GetNumberOfCells(), nPolyhedra);
  return true;
}

bool ValidatePolyhedra()
{
  try
  {
    // conduit data in host memory creates a VTK dataset so this test works.
    VERIFY(ValidatePolyhedraImpl(VISKORES_DEVICE_ADAPTER_SERIAL),
      "ValidateMeshTypeUnstructuredImpl with serial device failed.");
    // Viskores does not have VTK_POLYHEDRON
    // VERIFY(ValidatePolyhedraImpl(VISKORES_DEVICE_ADAPTER_CUDA),
    //   "ValidateMeshTypeUnstructuredImpl with CUDA device failed.");
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    std::cout << e.what() << std::endl;
  }
  return true;
}

} // end namespace

int TestConduitSourceDeviceMemory(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);
#if defined(VISKORES_ENABLE_CUDA)
  // We really want to use unmanaged memory to exercise external memory space code paths.
  SCOPED_CUDA_DISABLE_MANAGED_MEMORY;
#endif
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string amrFile =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Conduit/bp_amr_example.json");

  auto ret = ValidateMeshTypeStructured() && ValidateMeshTypeRectilinear() &&
      ValidateMeshTypeUnstructured() && ValidateRectilinearGridWithDifferentDimensions() &&
      Validate1DRectilinearGrid() && ValidateMeshTypeMixed() && ValidateMeshTypeMixed2D() &&
      ValidateMeshTypeAMR(amrFile) && ValidatePolyhedra()
    ? EXIT_SUCCESS
    : EXIT_FAILURE;

  controller->Finalize();

  return ret;
}
