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
#include <sstream>
#include <viskores/CellShape.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/internal/DeviceAdapterListHelpers.h>
#include <viskores/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/MeshConnectivity.h>
#include <viskores/rendering/raytracing/MeshConnectivityContainers.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>
namespace viskores
{
namespace rendering
{
namespace raytracing
{

MeshConnectivityContainer::MeshConnectivityContainer(){};
MeshConnectivityContainer::~MeshConnectivityContainer(){};

template <typename T>
VISKORES_CONT void MeshConnectivityContainer::FindEntryImpl(Ray<T>& rays)
{
  bool getCellIndex = true;

  Intersector.SetUseWaterTight(true);

  Intersector.IntersectRays(rays, getCellIndex);
}

void MeshConnectivityContainer::FindEntry(Ray<viskores::Float32>& rays)
{
  this->FindEntryImpl(rays);
}

void MeshConnectivityContainer::FindEntry(Ray<viskores::Float64>& rays)
{
  this->FindEntryImpl(rays);
}

VISKORES_CONT
MeshConnectivityContainerUnstructured::MeshConnectivityContainerUnstructured(
  const viskores::cont::CellSetExplicit<>& cellset,
  const viskores::cont::CoordinateSystem& coords,
  const IdHandle& faceConn,
  const IdHandle& faceOffsets,
  const Id4Handle& triangles)
  : FaceConnectivity(faceConn)
  , FaceOffsets(faceOffsets)
  , Cellset(cellset)
  , Coords(coords)
{
  this->Triangles = triangles;
  //
  // Grab the cell arrays
  //
  CellConn = Cellset.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                          viskores::TopologyElementTagPoint());
  CellOffsets = Cellset.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                        viskores::TopologyElementTagPoint());
  Shapes =
    Cellset.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

  Intersector.SetData(Coords, Triangles);
}

MeshConnectivityContainerUnstructured::~MeshConnectivityContainerUnstructured(){};

MeshConnectivity MeshConnectivityContainerUnstructured::PrepareForExecution(
  viskores::cont::DeviceAdapterId deviceId,
  viskores::cont::Token& token) const
{
  return MeshConnectivity(this->FaceConnectivity,
                          this->FaceOffsets,
                          this->CellConn,
                          this->CellOffsets,
                          this->Shapes,
                          deviceId,
                          token);
}

VISKORES_CONT
MeshConnectivityContainerSingleType::MeshConnectivityContainerSingleType(
  const viskores::cont::CellSetSingleType<>& cellset,
  const viskores::cont::CoordinateSystem& coords,
  const IdHandle& faceConn,
  const Id4Handle& triangles)
  : FaceConnectivity(faceConn)
  , Coords(coords)
  , Cellset(cellset)
{

  this->Triangles = triangles;

  this->Intersector.SetUseWaterTight(true);

  this->CellConnectivity = Cellset.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                        viskores::TopologyElementTagPoint());
  viskores::cont::ArrayHandleConstant<viskores::UInt8> shapes =
    Cellset.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

  this->ShapeId = shapes.ReadPortal().Get(0);
  CellTables tables;
  this->NumIndices = tables.FaceLookUp(tables.CellTypeLookUp(ShapeId), 2);

  if (this->NumIndices == 0)
  {
    std::stringstream message;
    message << "Unstructured Mesh Connecitity Single type Error: unsupported cell type: ";
    message << ShapeId;
    throw viskores::cont::ErrorBadValue(message.str());
  }
  viskores::Id start = 0;
  this->NumFaces = tables.FaceLookUp(tables.CellTypeLookUp(this->ShapeId), 1);
  viskores::Id numCells = this->CellConnectivity.ReadPortal().GetNumberOfValues();
  this->CellOffsets =
    viskores::cont::make_ArrayHandleCounting<viskores::Id>(start, this->NumIndices, numCells);

  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("mesh_conn_construction");

  this->Intersector.SetData(Coords, Triangles);
}

MeshConnectivity MeshConnectivityContainerSingleType::PrepareForExecution(
  viskores::cont::DeviceAdapterId deviceId,
  viskores::cont::Token& token) const
{
  return MeshConnectivity(this->FaceConnectivity,
                          this->CellConnectivity,
                          this->CellOffsets,
                          this->ShapeId,
                          this->NumIndices,
                          this->NumFaces,
                          deviceId,
                          token);
}

MeshConnectivityContainerStructured::MeshConnectivityContainerStructured(
  const viskores::cont::CellSetStructured<3>& cellset,
  const viskores::cont::CoordinateSystem& coords,
  const Id4Handle& triangles)
  : Coords(coords)
  , Cellset(cellset)
{

  this->Triangles = triangles;
  this->Intersector.SetUseWaterTight(true);

  this->PointDims = this->Cellset.GetPointDimensions();
  this->CellDims = this->Cellset.GetCellDimensions();

  this->Intersector.SetData(Coords, Triangles);
}

MeshConnectivity MeshConnectivityContainerStructured::PrepareForExecution(
  viskores::cont::DeviceAdapterId,
  viskores::cont::Token&) const
{
  return MeshConnectivity(CellDims, PointDims);
}
}
}
} //namespace viskores::rendering::raytracing
