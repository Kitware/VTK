//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_raytracing_MeshConnectivityContainer_h
#define viskores_rendering_raytracing_MeshConnectivityContainer_h

#include <viskores/cont/DataSet.h>
#include <viskores/rendering/raytracing/MeshConnectivity.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class MeshConnectivityContainer : viskores::cont::ExecutionObjectBase
{
public:
  MeshConnectivityContainer();
  virtual ~MeshConnectivityContainer();

  virtual MeshConnectivity PrepareForExecution(viskores::cont::DeviceAdapterId deviceId,
                                               viskores::cont::Token& token) const = 0;

  void FindEntry(Ray<viskores::Float32>& rays);

  void FindEntry(Ray<viskores::Float64>& rays);

protected:
  using Id4Handle = typename viskores::cont::ArrayHandle<viskores::Id4>;
  // Mesh Boundary
  Id4Handle Triangles;
  TriangleIntersector Intersector;

private:
  template <typename T>
  VISKORES_CONT void FindEntryImpl(Ray<T>& rays);
};

class MeshConnectivityContainerUnstructured : public MeshConnectivityContainer
{
public:
  typedef viskores::cont::ArrayHandle<viskores::Id> IdHandle;
  typedef viskores::cont::ArrayHandle<viskores::Id4> Id4Handle;
  typedef viskores::cont::ArrayHandle<viskores::UInt8> UCharHandle;
  // Control Environment Handles
  // FaceConn
  IdHandle FaceConnectivity;
  IdHandle FaceOffsets;
  //Cell Set
  IdHandle CellConn;
  IdHandle CellOffsets;
  UCharHandle Shapes;

  viskores::Bounds CoordinateBounds;
  viskores::cont::CellSetExplicit<> Cellset;
  viskores::cont::CoordinateSystem Coords;

public:
  VISKORES_CONT
  MeshConnectivityContainerUnstructured(const viskores::cont::CellSetExplicit<>& cellset,
                                        const viskores::cont::CoordinateSystem& coords,
                                        const IdHandle& faceConn,
                                        const IdHandle& faceOffsets,
                                        const Id4Handle& triangles);

  virtual ~MeshConnectivityContainerUnstructured();

  MeshConnectivity PrepareForExecution(viskores::cont::DeviceAdapterId deviceId,
                                       viskores::cont::Token& token) const override;
};

class MeshConnectivityContainerStructured : public MeshConnectivityContainer
{
protected:
  typedef viskores::cont::ArrayHandle<viskores::Id4> Id4Handle;
  viskores::Id3 CellDims;
  viskores::Id3 PointDims;
  viskores::Bounds CoordinateBounds;
  viskores::cont::CoordinateSystem Coords;
  viskores::cont::CellSetStructured<3> Cellset;

public:
  VISKORES_CONT
  MeshConnectivityContainerStructured(const viskores::cont::CellSetStructured<3>& cellset,
                                      const viskores::cont::CoordinateSystem& coords,
                                      const Id4Handle& triangles);

  MeshConnectivity PrepareForExecution(viskores::cont::DeviceAdapterId deviceId,
                                       viskores::cont::Token& token) const override;

}; //structure mesh conn

class MeshConnectivityContainerSingleType : public MeshConnectivityContainer
{
public:
  typedef viskores::cont::ArrayHandle<viskores::Id> IdHandle;
  typedef viskores::cont::ArrayHandle<viskores::Id4> Id4Handle;
  typedef viskores::cont::ArrayHandleCounting<viskores::Id> CountingHandle;
  typedef viskores::cont::ArrayHandleConstant<viskores::UInt8> ShapesHandle;
  typedef viskores::cont::ArrayHandleConstant<viskores::IdComponent> NumIndicesHandle;
  // Control Environment Handles
  IdHandle FaceConnectivity;
  CountingHandle CellOffsets;
  IdHandle CellConnectivity;

  viskores::Bounds CoordinateBounds;
  viskores::cont::CoordinateSystem Coords;
  viskores::cont::CellSetSingleType<> Cellset;

  viskores::Int32 ShapeId;
  viskores::Int32 NumIndices;
  viskores::Int32 NumFaces;

public:
  VISKORES_CONT
  MeshConnectivityContainerSingleType(const viskores::cont::CellSetSingleType<>& cellset,
                                      const viskores::cont::CoordinateSystem& coords,
                                      const IdHandle& faceConn,
                                      const Id4Handle& externalFaces);

  MeshConnectivity PrepareForExecution(viskores::cont::DeviceAdapterId deviceId,
                                       viskores::cont::Token& token) const override;

}; //UnstructuredSingleContainer
}
}
} //namespace viskores::rendering::raytracing
#endif
