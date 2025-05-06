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
#ifndef viskores_rendering_raytracing_MeshConnectivity
#define viskores_rendering_raytracing_MeshConnectivity

#include <sstream>
#include <viskores/CellShape.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/exec/Variant.h>
#include <viskores/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <viskores/rendering/raytracing/CellTables.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_ALWAYS_EXPORT MeshConnectivityStructured
{
protected:
  typedef typename viskores::cont::ArrayHandle<viskores::Id4> Id4Handle;
  viskores::Id3 CellDims;
  viskores::Id3 PointDims;

public:
  VISKORES_CONT
  MeshConnectivityStructured(const viskores::Id3& cellDims, const viskores::Id3& pointDims)
    : CellDims(cellDims)
    , PointDims(pointDims)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetConnectingCell(const viskores::Id& cellId, const viskores::Id& face) const
  {
    //TODO: there is probably a better way to do this.
    viskores::Id3 logicalCellId;
    logicalCellId[0] = cellId % CellDims[0];
    logicalCellId[1] = (cellId / CellDims[0]) % CellDims[1];
    logicalCellId[2] = cellId / (CellDims[0] * CellDims[1]);
    if (face == 0)
      logicalCellId[1] -= 1;
    if (face == 2)
      logicalCellId[1] += 1;
    if (face == 1)
      logicalCellId[0] += 1;
    if (face == 3)
      logicalCellId[0] -= 1;
    if (face == 4)
      logicalCellId[2] -= 1;
    if (face == 5)
      logicalCellId[2] += 1;
    viskores::Id nextCell =
      (logicalCellId[2] * CellDims[1] + logicalCellId[1]) * CellDims[0] + logicalCellId[0];
    bool validCell = true;
    if (logicalCellId[0] >= CellDims[0])
      validCell = false;
    if (logicalCellId[1] >= CellDims[1])
      validCell = false;
    if (logicalCellId[2] >= CellDims[2])
      validCell = false;
    viskores::Id minId =
      viskores::Min(logicalCellId[0], viskores::Min(logicalCellId[1], logicalCellId[2]));
    if (minId < 0)
      validCell = false;
    if (!validCell)
      nextCell = -1;
    return nextCell;
  }

  VISKORES_EXEC_CONT
  viskores::Int32 GetCellIndices(viskores::Id cellIndices[8], const viskores::Id& cellIndex) const
  {
    viskores::Id3 cellId;
    cellId[0] = cellIndex % CellDims[0];
    cellId[1] = (cellIndex / CellDims[0]) % CellDims[1];
    cellId[2] = cellIndex / (CellDims[0] * CellDims[1]);
    cellIndices[0] = (cellId[2] * PointDims[1] + cellId[1]) * PointDims[0] + cellId[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDims[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDims[0] * PointDims[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDims[0];
    cellIndices[7] = cellIndices[6] - 1;
    return 8;
  }

  VISKORES_EXEC
  viskores::UInt8 GetCellShape(const viskores::Id& viskoresNotUsed(cellId)) const
  {
    return viskores::UInt8(CELL_SHAPE_HEXAHEDRON);
  }
}; // MeshConnStructured

class VISKORES_ALWAYS_EXPORT MeshConnectivityUnstructured
{
protected:
  using IdHandle = typename viskores::cont::ArrayHandle<viskores::Id>;
  using UCharHandle = typename viskores::cont::ArrayHandle<viskores::UInt8>;
  using IdConstPortal = typename IdHandle::ReadPortalType;
  using UCharConstPortal = typename UCharHandle::ReadPortalType;

  // Constant Portals for the execution Environment
  //FaceConn
  IdConstPortal FaceConnPortal;
  IdConstPortal FaceOffsetsPortal;
  //Cell Set
  IdConstPortal CellConnPortal;
  IdConstPortal CellOffsetsPortal;
  UCharConstPortal ShapesPortal;

public:
  VISKORES_CONT
  MeshConnectivityUnstructured(const IdHandle& faceConnectivity,
                               const IdHandle& faceOffsets,
                               const IdHandle& cellConn,
                               const IdHandle& cellOffsets,
                               const UCharHandle& shapes,
                               viskores::cont::DeviceAdapterId device,
                               viskores::cont::Token& token)
    : FaceConnPortal(faceConnectivity.PrepareForInput(device, token))
    , FaceOffsetsPortal(faceOffsets.PrepareForInput(device, token))
    , CellConnPortal(cellConn.PrepareForInput(device, token))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(device, token))
    , ShapesPortal(shapes.PrepareForInput(device, token))
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetConnectingCell(const viskores::Id& cellId, const viskores::Id& face) const
  {
    BOUNDS_CHECK(FaceOffsetsPortal, cellId);
    viskores::Id cellStartIndex = FaceOffsetsPortal.Get(cellId);
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  //----------------------------------------------------------------------------
  VISKORES_EXEC
  viskores::Int32 GetCellIndices(viskores::Id cellIndices[8], const viskores::Id& cellId) const
  {
    const viskores::Int32 shapeId = static_cast<viskores::Int32>(ShapesPortal.Get(cellId));
    CellTables tables;
    const viskores::Int32 numIndices = tables.FaceLookUp(tables.CellTypeLookUp(shapeId), 2);
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    const viskores::Id cellOffset = CellOffsetsPortal.Get(cellId);

    for (viskores::Int32 i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(CellConnPortal, cellOffset + i);
      cellIndices[i] = CellConnPortal.Get(cellOffset + i);
    }
    return numIndices;
  }

  //----------------------------------------------------------------------------
  VISKORES_EXEC
  viskores::UInt8 GetCellShape(const viskores::Id& cellId) const
  {
    BOUNDS_CHECK(ShapesPortal, cellId);
    return ShapesPortal.Get(cellId);
  }

}; // MeshConnUnstructured

class MeshConnectivitySingleType
{
protected:
  using IdHandle = typename viskores::cont::ArrayHandle<viskores::Id>;
  using IdConstPortal = typename IdHandle::ReadPortalType;

  using CountingHandle = typename viskores::cont::ArrayHandleCounting<viskores::Id>;
  using CountingPortal = typename CountingHandle::ReadPortalType;
  // Constant Portals for the execution Environment
  IdConstPortal FaceConnPortal;
  IdConstPortal CellConnectivityPortal;
  CountingPortal CellOffsetsPortal;

  viskores::Int32 ShapeId;
  viskores::Int32 NumIndices;
  viskores::Int32 NumFaces;

public:
  VISKORES_CONT
  MeshConnectivitySingleType(const IdHandle& faceConn,
                             const IdHandle& cellConn,
                             const CountingHandle& cellOffsets,
                             viskores::Int32 shapeId,
                             viskores::Int32 numIndices,
                             viskores::Int32 numFaces,
                             viskores::cont::DeviceAdapterId device,
                             viskores::cont::Token& token)
    : FaceConnPortal(faceConn.PrepareForInput(device, token))
    , CellConnectivityPortal(cellConn.PrepareForInput(device, token))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(device, token))
    , ShapeId(shapeId)
    , NumIndices(numIndices)
    , NumFaces(numFaces)
  {
  }

  //----------------------------------------------------------------------------
  //                       Execution Environment Methods
  //----------------------------------------------------------------------------
  VISKORES_EXEC
  viskores::Id GetConnectingCell(const viskores::Id& cellId, const viskores::Id& face) const
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    viskores::Id cellStartIndex = cellId * NumFaces;
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  VISKORES_EXEC
  viskores::Int32 GetCellIndices(viskores::Id cellIndices[8], const viskores::Id& cellId) const
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    const viskores::Id cellOffset = CellOffsetsPortal.Get(cellId);

    for (viskores::Int32 i = 0; i < NumIndices; ++i)
    {
      BOUNDS_CHECK(CellConnectivityPortal, cellOffset + i);
      cellIndices[i] = CellConnectivityPortal.Get(cellOffset + i);
    }

    return NumIndices;
  }

  //----------------------------------------------------------------------------
  VISKORES_EXEC
  viskores::UInt8 GetCellShape(const viskores::Id& viskoresNotUsed(cellId)) const
  {
    return viskores::UInt8(ShapeId);
  }

}; //MeshConn Single type specialization

/// \brief General version of mesh connectivity that can be used for all supported mesh types.
class VISKORES_ALWAYS_EXPORT MeshConnectivity
{
  using ConnectivityType = viskores::exec::
    Variant<MeshConnectivityStructured, MeshConnectivityUnstructured, MeshConnectivitySingleType>;
  ConnectivityType Connectivity;

public:
  // Constructor for structured connectivity
  VISKORES_CONT MeshConnectivity(const viskores::Id3& cellDims, const viskores::Id3& pointDims)
    : Connectivity(MeshConnectivityStructured(cellDims, pointDims))
  {
  }

  // Constructor for unstructured connectivity
  VISKORES_CONT MeshConnectivity(const viskores::cont::ArrayHandle<viskores::Id>& faceConnectivity,
                                 const viskores::cont::ArrayHandle<viskores::Id>& faceOffsets,
                                 const viskores::cont::ArrayHandle<viskores::Id>& cellConn,
                                 const viskores::cont::ArrayHandle<viskores::Id>& cellOffsets,
                                 const viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                                 viskores::cont::DeviceAdapterId device,
                                 viskores::cont::Token& token)
    : Connectivity(MeshConnectivityUnstructured(faceConnectivity,
                                                faceOffsets,
                                                cellConn,
                                                cellOffsets,
                                                shapes,
                                                device,
                                                token))
  {
  }

  // Constructor for unstructured connectivity with single cell type
  VISKORES_CONT MeshConnectivity(
    const viskores::cont::ArrayHandle<viskores::Id>& faceConn,
    const viskores::cont::ArrayHandle<viskores::Id>& cellConn,
    const viskores::cont::ArrayHandleCounting<viskores::Id>& cellOffsets,
    viskores::Int32 shapeId,
    viskores::Int32 numIndices,
    viskores::Int32 numFaces,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : Connectivity(MeshConnectivitySingleType(faceConn,
                                              cellConn,
                                              cellOffsets,
                                              shapeId,
                                              numIndices,
                                              numFaces,
                                              device,
                                              token))
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetConnectingCell(const viskores::Id& cellId, const viskores::Id& face) const
  {
    return this->Connectivity.CastAndCall([=](auto conn)
                                          { return conn.GetConnectingCell(cellId, face); });
  }

  VISKORES_EXEC_CONT
  viskores::Int32 GetCellIndices(viskores::Id cellIndices[8], const viskores::Id& cellId) const
  {
    return this->Connectivity.CastAndCall([=](auto conn)
                                          { return conn.GetCellIndices(cellIndices, cellId); });
  }

  VISKORES_EXEC_CONT
  viskores::UInt8 GetCellShape(const viskores::Id& cellId) const
  {
    return this->Connectivity.CastAndCall([=](auto conn) { return conn.GetCellShape(cellId); });
  }
};

}
}
} //namespace viskores::rendering::raytracing


#endif // MeshConnectivity
