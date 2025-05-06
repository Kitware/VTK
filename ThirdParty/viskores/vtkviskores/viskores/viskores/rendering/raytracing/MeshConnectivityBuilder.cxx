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
#include <viskores/rendering/raytracing/MeshConnectivityBuilder.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/Timer.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/rendering/raytracing/CellTables.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/MortonCodes.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/rendering/raytracing/Worklets.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

struct IsUnique
{
  VISKORES_EXEC_CONT
  inline bool operator()(const viskores::Int32& x) const { return x < 0; }
}; //struct IsExternal

class CountFaces : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CountFaces() {}
  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename ShapePortalType>
  VISKORES_EXEC inline void operator()(const ShapePortalType& shapes,
                                       viskores::Id& faces,
                                       const viskores::Id& index) const
  {
    BOUNDS_CHECK(shapes, index);
    viskores::UInt8 shapeType = shapes.Get(index);
    if (shapeType == viskores::CELL_SHAPE_TETRA)
    {
      faces = 4;
    }
    else if (shapeType == viskores::CELL_SHAPE_HEXAHEDRON)
    {
      faces = 6;
    }
    else if (shapeType == viskores::CELL_SHAPE_WEDGE)
    {
      faces = 5;
    }
    else if (shapeType == viskores::CELL_SHAPE_PYRAMID)
    {
      faces = 5;
    }
    else
    {
      faces = 0;
    }
  }
}; //class CountFaces

class MortonNeighbor : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  MortonNeighbor() {}
  using ControlSignature = void(WholeArrayIn,
                                WholeArrayInOut,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayOut,
                                WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, WorkIndex, _3, _4, _5, _6, _7);

  VISKORES_EXEC
  inline viskores::Int32 GetShapeOffset(const viskores::UInt8& shapeType) const
  {

    CellTables tables;
    //TODO: This might be better as if if if
    viskores::Int32 tableOffset = 0;
    if (shapeType == viskores::CELL_SHAPE_TETRA)
    {
      tableOffset = tables.FaceLookUp(1, 0);
    }
    else if (shapeType == viskores::CELL_SHAPE_HEXAHEDRON)
    {
      tableOffset = tables.FaceLookUp(0, 0);
    }
    else if (shapeType == viskores::CELL_SHAPE_WEDGE)
    {
      tableOffset = tables.FaceLookUp(2, 0);
    }
    else if (shapeType == viskores::CELL_SHAPE_PYRAMID)
    {
      tableOffset = tables.FaceLookUp(3, 0);
    }

    return tableOffset;
  }

  VISKORES_EXEC
  inline bool IsIn(const viskores::Id& needle,
                   const viskores::Id4& heystack,
                   const viskores::Int32& numIndices) const
  {
    bool isIn = false;
    for (viskores::Int32 i = 0; i < numIndices; ++i)
    {
      if (needle == heystack[i])
        isIn = true;
    }
    return isIn;
  }


  template <typename MortonPortalType,
            typename FaceIdPairsPortalType,
            typename ConnPortalType,
            typename ShapePortalType,
            typename OffsetPortalType,
            typename ExternalFaceFlagType,
            typename UniqueFacesType>
  VISKORES_EXEC inline void operator()(const MortonPortalType& mortonCodes,
                                       FaceIdPairsPortalType& faceIdPairs,
                                       const viskores::Id& index,
                                       const ConnPortalType& connectivity,
                                       const ShapePortalType& shapes,
                                       const OffsetPortalType& offsets,
                                       ExternalFaceFlagType& flags,
                                       UniqueFacesType& uniqueFaces) const
  {
    if (index == 0)
    {
      return;
    }

    viskores::Id currentIndex = index - 1;
    BOUNDS_CHECK(mortonCodes, index);
    viskores::UInt32 myCode = mortonCodes.Get(index);
    BOUNDS_CHECK(mortonCodes, currentIndex);
    viskores::UInt32 myNeighbor = mortonCodes.Get(currentIndex);
    bool isInternal = false;
    viskores::Id connectedCell = -1;

    CellTables tables;
    while (currentIndex > -1 && myCode == myNeighbor)
    {
      myNeighbor = mortonCodes.Get(currentIndex);
      // just because the codes are equal does not mean that
      // they are the same face. We need to double check.
      if (myCode == myNeighbor)
      {
        //get the index of the shape face in the table.
        BOUNDS_CHECK(faceIdPairs, index);
        viskores::Id cellId1 = faceIdPairs.Get(index)[0];
        BOUNDS_CHECK(faceIdPairs, currentIndex);
        viskores::Id cellId2 = faceIdPairs.Get(currentIndex)[0];
        BOUNDS_CHECK(shapes, cellId1);
        BOUNDS_CHECK(shapes, cellId2);
        viskores::Int32 shape1Offset = GetShapeOffset(shapes.Get(cellId1)) +
          static_cast<viskores::Int32>(faceIdPairs.Get(index)[1]);
        viskores::Int32 shape2Offset = GetShapeOffset(shapes.Get(cellId2)) +
          static_cast<viskores::Int32>(faceIdPairs.Get(currentIndex)[1]);

        const viskores::Int32 icount1 = tables.ShapesFaceList(shape1Offset, 0);
        const viskores::Int32 icount2 = tables.ShapesFaceList(shape2Offset, 0);
        //Check to see if we have the same number of idices
        if (icount1 != icount2)
          continue;


        //TODO: we can do better than this
        viskores::Id4 indices1;
        viskores::Id4 indices2;

        const auto faceLength = tables.ShapesFaceList(shape1Offset, 0);
        for (viskores::Int32 i = 1; i <= faceLength; ++i)
        {
          BOUNDS_CHECK(offsets, cellId1);
          BOUNDS_CHECK(offsets, cellId2);
          BOUNDS_CHECK(connectivity,
                       (offsets.Get(cellId1) + tables.ShapesFaceList(shape1Offset, i)));
          BOUNDS_CHECK(connectivity,
                       (offsets.Get(cellId2) + tables.ShapesFaceList(shape2Offset, i)));
          indices1[i - 1] =
            connectivity.Get(offsets.Get(cellId1) + tables.ShapesFaceList(shape1Offset, i));
          indices2[i - 1] =
            connectivity.Get(offsets.Get(cellId2) + tables.ShapesFaceList(shape2Offset, i));
        }

        bool isEqual = true;
        for (viskores::Int32 i = 0; i < faceLength; ++i)
        {
          if (!IsIn(indices1[i], indices2, faceLength))
            isEqual = false;
        }

        if (isEqual)
        {
          isInternal = true;

          connectedCell = cellId2;

          break;
        }
      }
      currentIndex--;
    }

    //this means that this cell is responsible for both itself and the other cell
    //set the connection for the other cell
    if (isInternal)
    {
      BOUNDS_CHECK(faceIdPairs, index);
      viskores::Id3 facePair = faceIdPairs.Get(index);
      viskores::Id myCell = facePair[0];
      facePair[2] = connectedCell;
      BOUNDS_CHECK(faceIdPairs, index);
      faceIdPairs.Set(index, facePair);

      BOUNDS_CHECK(faceIdPairs, currentIndex);
      facePair = faceIdPairs.Get(currentIndex);
      facePair[2] = myCell;
      BOUNDS_CHECK(faceIdPairs, currentIndex);
      faceIdPairs.Set(currentIndex, facePair);
      BOUNDS_CHECK(flags, currentIndex);
      flags.Set(currentIndex, myCell);
      BOUNDS_CHECK(flags, index);
      flags.Set(index, connectedCell);

      // for unstructured, we want all unique faces to intersect with
      // just choose one and mark as unique so the other gets culled.
      uniqueFaces.Set(index, 1);
    }
  }
}; //class Neighbor

class ExternalTriangles : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  ExternalTriangles() {}
  using ControlSignature =
    void(FieldIn, WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  template <typename ShapePortalType,
            typename InIndicesPortalType,
            typename OutIndicesPortalType,
            typename ShapeOffsetsPortal>
  VISKORES_EXEC inline void operator()(const viskores::Id3& faceIdPair,
                                       const ShapePortalType& shapes,
                                       const ShapeOffsetsPortal& shapeOffsets,
                                       const InIndicesPortalType& indices,
                                       const OutIndicesPortalType& outputIndices,
                                       const viskores::Id& outputOffset) const
  {
    CellTables tables;

    viskores::Id cellId = faceIdPair[0];
    BOUNDS_CHECK(shapeOffsets, cellId);
    viskores::Id offset = shapeOffsets.Get(cellId);
    BOUNDS_CHECK(shapes, cellId);
    viskores::Int32 shapeId = static_cast<viskores::Int32>(shapes.Get(cellId));
    viskores::Int32 shapesFaceOffset = tables.FaceLookUp(tables.CellTypeLookUp(shapeId), 0);
    if (shapesFaceOffset == -1)
    {
      return;
    }

    viskores::Id4 faceIndices(-1, -1, -1, -1);
    viskores::Int32 tableIndex = static_cast<viskores::Int32>(shapesFaceOffset + faceIdPair[1]);
    const viskores::Int32 numIndices = tables.ShapesFaceList(tableIndex, 0);

    for (viskores::Int32 i = 1; i <= numIndices; ++i)
    {
      BOUNDS_CHECK(indices, offset + tables.ShapesFaceList(tableIndex, i));
      faceIndices[i - 1] = indices.Get(offset + tables.ShapesFaceList(tableIndex, i));
    }
    viskores::Id4 triangle;
    triangle[0] = cellId;
    triangle[1] = faceIndices[0];
    triangle[2] = faceIndices[1];
    triangle[3] = faceIndices[2];
    BOUNDS_CHECK(outputIndices, outputOffset);
    outputIndices.Set(outputOffset, triangle);
    if (numIndices == 4)
    {
      triangle[1] = faceIndices[0];
      triangle[2] = faceIndices[2];
      triangle[3] = faceIndices[3];

      BOUNDS_CHECK(outputIndices, outputOffset + 1);
      outputIndices.Set(outputOffset + 1, triangle);
    }
  }
}; //class External Triangles

// Face conn was originally used for filtering out internal
// faces and was sorted with faces. To make it usable,
// we need to scatter back the connectivity into the original
// cell order, i.e., conn for cell 0 at 0,1,2,3,4,5
class WriteFaceConn : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  WriteFaceConn() {}

  template <typename FaceOffsetsPortalType, typename FaceConnectivityPortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id3& faceIdPair,
                                       const FaceOffsetsPortalType& faceOffsets,
                                       FaceConnectivityPortalType& faceConn) const
  {
    viskores::Id cellId = faceIdPair[0];
    BOUNDS_CHECK(faceOffsets, cellId);
    viskores::Id faceOffset = faceOffsets.Get(cellId) + faceIdPair[1]; // cellOffset ++ faceId
    BOUNDS_CHECK(faceConn, faceOffset);
    faceConn.Set(faceOffset, faceIdPair[2]);
  }
}; //class WriteFaceConn

class StructuredExternalTriangles : public viskores::worklet::WorkletMapField
{
protected:
  using ConnType = viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 3>;
  ConnType Connectivity;
  viskores::Id Segments[7];
  viskores::Id3 CellDims;

public:
  VISKORES_CONT
  StructuredExternalTriangles(const ConnType& connectivity)
    : Connectivity(connectivity)
  {
    viskores::Id3 cellDims = Connectivity.GetPointDimensions();
    cellDims[0] -= 1;
    cellDims[1] -= 1;
    cellDims[2] -= 1;
    CellDims = cellDims;

    //We have 6 segments for each of the six faces.
    Segments[0] = 0;
    // 0-1 = the two faces parallel to the x-z plane
    Segments[1] = cellDims[0] * cellDims[2];
    Segments[2] = Segments[1] + Segments[1];
    // 2-3 parallel to the y-z plane
    Segments[3] = Segments[2] + cellDims[1] * cellDims[2];
    Segments[4] = Segments[3] + cellDims[1] * cellDims[2];
    // 4-5 parallel to the x-y plane
    Segments[5] = Segments[4] + cellDims[1] * cellDims[0];
    Segments[6] = Segments[5] + cellDims[1] * cellDims[0];
  }
  using ControlSignature = void(FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2);
  template <typename TrianglePortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id& index,
                                       TrianglePortalType& triangles) const
  {
    // Each one of size segments will process
    // one face of the hex and domain
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 SegmentToFace[6] = { 0, 2, 1, 3, 4, 5 };

    // Each face/segment has 2 varying dimensions
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 SegmentDirections[6][2] = {
      { 0, 2 },           //face 0 and 2 have the same directions
      { 0, 2 }, { 1, 2 }, //1 and 3
      { 1, 2 }, { 0, 1 }, // 4 and 5
      { 0, 1 }
    };

    //
    // We get one index per external face
    //

    //
    // Cell face to extract which is also the domain
    // face in this segment
    //

    viskores::Int32 segment;

    for (segment = 0; index >= Segments[segment + 1]; ++segment)
      ;

    viskores::Int32 cellFace = SegmentToFace[segment];

    // Face relative directions of the
    // 2 varying coordinates.
    viskores::Int32 dir1, dir2;
    dir1 = SegmentDirections[segment][0];
    dir2 = SegmentDirections[segment][1];

    // For each face, we will have a relative offset to
    // the "bottom corner of the face. Three are at the
    // origin. and we have to adjust for the other faces.
    viskores::Id3 cellIndex(0, 0, 0);
    if (cellFace == 1)
      cellIndex[0] = CellDims[0] - 1;
    if (cellFace == 2)
      cellIndex[1] = CellDims[1] - 1;
    if (cellFace == 5)
      cellIndex[2] = CellDims[2] - 1;


    // index is the global index of all external faces
    // the offset is the relative index of the cell
    // on the current 2d face

    viskores::Id offset = index - Segments[segment];
    viskores::Id dir1Offset = offset % CellDims[dir1];
    viskores::Id dir2Offset = offset / CellDims[dir1];

    cellIndex[dir1] = cellIndex[dir1] + dir1Offset;
    cellIndex[dir2] = cellIndex[dir2] + dir2Offset;
    viskores::Id cellId = Connectivity.LogicalToFlatVisitIndex(cellIndex);
    viskores::VecVariable<viskores::Id, 8> cellIndices = Connectivity.GetIndices(cellId);

    // Look up the offset into the face list for each cell type
    // This should always be zero, but in case this table changes I don't
    // want to break anything.
    CellTables tables;
    viskores::Int32 shapesFaceOffset =
      tables.FaceLookUp(tables.CellTypeLookUp(CELL_SHAPE_HEXAHEDRON), 0);

    viskores::Id4 faceIndices;
    viskores::Int32 tableIndex = shapesFaceOffset + cellFace;

    // Load the face
    for (viskores::Int32 i = 1; i <= 4; ++i)
    {
      faceIndices[i - 1] = cellIndices[tables.ShapesFaceList(tableIndex, i)];
    }
    const viskores::Id outputOffset = index * 2;
    viskores::Id4 triangle;
    triangle[0] = cellId;
    triangle[1] = faceIndices[0];
    triangle[2] = faceIndices[1];
    triangle[3] = faceIndices[2];
    BOUNDS_CHECK(triangles, outputOffset);
    triangles.Set(outputOffset, triangle);
    triangle[1] = faceIndices[0];
    triangle[2] = faceIndices[2];
    triangle[3] = faceIndices[3];
    BOUNDS_CHECK(triangles, outputOffset);
    triangles.Set(outputOffset + 1, triangle);
  }
}; //class  StructuredExternalTriangles

//If with output faces or triangles, we still have to calculate the size of the output
//array. TODO: switch to faces only
class CountExternalTriangles : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CountExternalTriangles() {}
  using ControlSignature = void(FieldIn, WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);
  template <typename ShapePortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id3& faceIdPair,
                                       const ShapePortalType& shapes,
                                       viskores::Id& triangleCount) const
  {
    CellTables tables;
    viskores::Id cellId = faceIdPair[0];
    viskores::Id cellFace = faceIdPair[1];
    viskores::Int32 shapeType = static_cast<viskores::Int32>(shapes.Get(cellId));
    viskores::Int32 faceStartIndex = tables.FaceLookUp(tables.CellTypeLookUp(shapeType), 0);
    if (faceStartIndex == -1)
    {
      //Unsupported Shape Type this should never happen
      triangleCount = 0;
      return;
    }
    viskores::Int32 faceType =
      tables.ShapesFaceList(faceStartIndex + static_cast<viskores::Int32>(cellFace), 0);
    // The face will either have 4 or 3 indices, so quad or tri
    triangleCount = (faceType == 4) ? 2 : 1;

    //faceConn.Set(faceOffset, faceIdPair[2]);
  }
}; //class WriteFaceConn

template <typename CellSetType,
          typename ShapeHandleType,
          typename ConnHandleType,
          typename OffsetsHandleType,
          typename CoordsType>
VISKORES_CONT void GenerateFaceConnnectivity(
  const CellSetType cellSet,
  const ShapeHandleType shapes,
  const ConnHandleType conn,
  const OffsetsHandleType shapeOffsets,
  const CoordsType& coords,
  viskores::cont::ArrayHandle<viskores::Id>& faceConnectivity,
  viskores::cont::ArrayHandle<viskores::Id3>& cellFaceId,
  viskores::Float32 BoundingBox[6],
  viskores::cont::ArrayHandle<viskores::Id>& faceOffsets,
  viskores::cont::ArrayHandle<viskores::Int32>& uniqueFaces)
{

  viskores::cont::Timer timer;
  timer.Start();

  viskores::Id numCells = shapes.GetNumberOfValues();

  viskores::cont::ArrayHandle<viskores::Vec3f> coordinates;
  viskores::cont::Algorithm::Copy(coords, coordinates);

  /*-----------------------------------------------------------------*/

  // Count the number of total faces in the cell set
  viskores::cont::ArrayHandle<viskores::Id> facesPerCell;

  viskores::worklet::DispatcherMapField<CountFaces>(CountFaces()).Invoke(shapes, facesPerCell);

  viskores::Id totalFaces = 0;
  totalFaces = viskores::cont::Algorithm::Reduce(facesPerCell, viskores::Id(0));

  // Calculate the offsets so each cell knows where to insert the morton code
  // for each face
  viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
  cellOffsets.Allocate(numCells);
  viskores::cont::Algorithm::ScanExclusive(facesPerCell, cellOffsets);
  // cell offsets also serve as the offsets into the array that tracks connectivity.
  // For example, we have a hex with 6 faces and each face connects to another cell.
  // The connecting cells (from each face) are stored beginning at index cellOffsets[cellId]
  faceOffsets = cellOffsets;

  // We are creating a spatial hash based on morton codes calculated
  // from the centriod (point average)  of each face. Each centroid is
  // calculated in way (consistent order of floating point calcs) that
  // ensures that each face maps to the same morton code. It is possbilbe
  // that two non-connecting faces map to the same morton code,  but if
  // if a face has a matching face from another cell, it will be mapped
  // to the same morton code. We check for this.

  // set up everything we need to gen morton codes
  viskores::Vec3f_32 inverseExtent;
  inverseExtent[0] = 1.f / (BoundingBox[1] - BoundingBox[0]);
  inverseExtent[1] = 1.f / (BoundingBox[3] - BoundingBox[2]);
  inverseExtent[2] = 1.f / (BoundingBox[5] - BoundingBox[4]);
  viskores::Vec3f_32 minPoint;
  minPoint[0] = BoundingBox[0];
  minPoint[1] = BoundingBox[2];
  minPoint[2] = BoundingBox[4];

  // Morton Codes are created for the centroid of each face.
  // cellFaceId:
  //  0) Cell that the face belongs to
  //  1) Face of the the cell (e.g., hex will have 6 faces and this is 1 of the 6)
  //  2) cell id of the cell that connects to the corresponding face (1)
  viskores::cont::ArrayHandle<viskores::UInt32> faceMortonCodes;
  cellFaceId.Allocate(totalFaces);
  faceMortonCodes.Allocate(totalFaces);
  uniqueFaces.Allocate(totalFaces);

  viskores::worklet::DispatcherMapTopology<MortonCodeFace>(MortonCodeFace(inverseExtent, minPoint))
    .Invoke(cellSet, coordinates, cellOffsets, faceMortonCodes, cellFaceId);

  // Sort the "faces" (i.e., morton codes)
  viskores::cont::Algorithm::SortByKey(faceMortonCodes, cellFaceId);

  // Allocate space for the final face-to-face conectivity
  //faceConnectivity.PrepareForOutput(totalFaces, DeviceAdapter());
  faceConnectivity.Allocate(totalFaces);

  // Initialize All connecting faces to -1 (connects to nothing)
  viskores::cont::ArrayHandleConstant<viskores::Id> negOne(-1, totalFaces);
  viskores::cont::Algorithm::Copy(negOne, faceConnectivity);

  viskores::cont::ArrayHandleConstant<viskores::Int32> negOne32(-1, totalFaces);
  viskores::cont::Algorithm::Copy(negOne32, uniqueFaces);

  viskores::worklet::DispatcherMapField<MortonNeighbor>(MortonNeighbor())
    .Invoke(faceMortonCodes, cellFaceId, conn, shapes, shapeOffsets, faceConnectivity, uniqueFaces);

  viskores::Float64 time = timer.GetElapsedTime();
  Logger::GetInstance()->AddLogData("gen_face_conn", time);
}

template <typename ShapeHandleType, typename OffsetsHandleType, typename ConnHandleType>
VISKORES_CONT viskores::cont::ArrayHandle<viskores::Vec<Id, 4>> ExtractFaces(
  viskores::cont::ArrayHandle<viskores::Id3> cellFaceId, // Map of cell, face, and connecting cell
  viskores::cont::ArrayHandle<viskores::Int32> uniqueFaces, // -1 if the face is unique
  const ShapeHandleType& shapes,
  const ConnHandleType& conn,
  const OffsetsHandleType& shapeOffsets)
{

  viskores::cont::Timer timer;
  timer.Start();
  viskores::cont::ArrayHandle<viskores::Id3> externalFacePairs;
  viskores::cont::Algorithm::CopyIf(cellFaceId, uniqueFaces, externalFacePairs, IsUnique());

  // We need to count the number of triangle per external face
  // If it is a single cell type and it is a tet or hex, this is a special case
  // i.e., we do not need to calculate it. TODO
  // Otherwise, we need to check to see if the face is a quad or triangle

  viskores::Id numExternalFaces = externalFacePairs.GetNumberOfValues();

  viskores::cont::ArrayHandle<viskores::Id> trianglesPerExternalFace;
  //trianglesPerExternalFace.PrepareForOutput(numExternalFaces, DeviceAdapter());
  trianglesPerExternalFace.Allocate(numExternalFaces);


  viskores::worklet::DispatcherMapField<CountExternalTriangles>(CountExternalTriangles())
    .Invoke(externalFacePairs, shapes, trianglesPerExternalFace);

  viskores::cont::ArrayHandle<viskores::Id> externalTriangleOffsets;
  viskores::cont::Algorithm::ScanExclusive(trianglesPerExternalFace, externalTriangleOffsets);

  viskores::Id totalExternalTriangles;
  totalExternalTriangles =
    viskores::cont::Algorithm::Reduce(trianglesPerExternalFace, viskores::Id(0));
  viskores::cont::ArrayHandle<viskores::Id4> externalTriangles;
  //externalTriangles.PrepareForOutput(totalExternalTriangles, DeviceAdapter());
  externalTriangles.Allocate(totalExternalTriangles);
  //count the number triangles in the external faces

  viskores::worklet::DispatcherMapField<ExternalTriangles>(ExternalTriangles())
    .Invoke(
      externalFacePairs, shapes, shapeOffsets, conn, externalTriangles, externalTriangleOffsets);


  viskores::Float64 time = timer.GetElapsedTime();
  Logger::GetInstance()->AddLogData("external_faces", time);
  return externalTriangles;
}

MeshConnectivityBuilder::MeshConnectivityBuilder() {}
MeshConnectivityBuilder::~MeshConnectivityBuilder() {}


VISKORES_CONT
void MeshConnectivityBuilder::BuildConnectivity(
  viskores::cont::CellSetSingleType<>& cellSetUnstructured,
  const viskores::cont::CoordinateSystem::MultiplexerArrayType& coordinates,
  viskores::Bounds coordsBounds)
{
  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("mesh_conn");
  //logger->AddLogData("device", GetDeviceString(DeviceAdapter()));
  viskores::cont::Timer timer;
  timer.Start();

  viskores::Float32 BoundingBox[6];
  BoundingBox[0] = viskores::Float32(coordsBounds.X.Min);
  BoundingBox[1] = viskores::Float32(coordsBounds.X.Max);
  BoundingBox[2] = viskores::Float32(coordsBounds.Y.Min);
  BoundingBox[3] = viskores::Float32(coordsBounds.Y.Max);
  BoundingBox[4] = viskores::Float32(coordsBounds.Z.Min);
  BoundingBox[5] = viskores::Float32(coordsBounds.Z.Max);

  const viskores::cont::ArrayHandleConstant<viskores::UInt8> shapes =
    cellSetUnstructured.GetShapesArray(viskores::TopologyElementTagCell(),
                                       viskores::TopologyElementTagPoint());

  const viskores::cont::ArrayHandle<viskores::Id> conn = cellSetUnstructured.GetConnectivityArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

  const viskores::cont::ArrayHandleCounting<viskores::Id> offsets =
    cellSetUnstructured.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                        viskores::TopologyElementTagPoint());
  const auto shapeOffsets =
    viskores::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);

  viskores::cont::ArrayHandle<viskores::Id> faceConnectivity;
  viskores::cont::ArrayHandle<viskores::Id3> cellFaceId;
  viskores::cont::ArrayHandle<viskores::Int32> uniqueFaces;

  GenerateFaceConnnectivity(cellSetUnstructured,
                            shapes,
                            conn,
                            shapeOffsets,
                            coordinates,
                            faceConnectivity,
                            cellFaceId,
                            BoundingBox,
                            FaceOffsets,
                            uniqueFaces);

  viskores::cont::ArrayHandle<viskores::Id4> triangles;
  //Faces

  triangles = ExtractFaces(cellFaceId, uniqueFaces, shapes, conn, shapeOffsets);


  // scatter the coonectivity into the original order
  viskores::worklet::DispatcherMapField<WriteFaceConn>(WriteFaceConn())
    .Invoke(cellFaceId, this->FaceOffsets, faceConnectivity);

  FaceConnectivity = faceConnectivity;
  Triangles = triangles;

  viskores::Float64 time = timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

VISKORES_CONT
void MeshConnectivityBuilder::BuildConnectivity(
  viskores::cont::CellSetExplicit<>& cellSetUnstructured,
  const viskores::cont::CoordinateSystem::MultiplexerArrayType& coordinates,
  viskores::Bounds coordsBounds)
{
  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("meah_conn");
  viskores::cont::Timer timer;
  timer.Start();

  viskores::Float32 BoundingBox[6];
  BoundingBox[0] = viskores::Float32(coordsBounds.X.Min);
  BoundingBox[1] = viskores::Float32(coordsBounds.X.Max);
  BoundingBox[2] = viskores::Float32(coordsBounds.Y.Min);
  BoundingBox[3] = viskores::Float32(coordsBounds.Y.Max);
  BoundingBox[4] = viskores::Float32(coordsBounds.Z.Min);
  BoundingBox[5] = viskores::Float32(coordsBounds.Z.Max);

  const viskores::cont::ArrayHandle<viskores::UInt8> shapes = cellSetUnstructured.GetShapesArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

  const viskores::cont::ArrayHandle<viskores::Id> conn = cellSetUnstructured.GetConnectivityArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

  const viskores::cont::ArrayHandle<viskores::Id> offsets = cellSetUnstructured.GetOffsetsArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  const auto shapeOffsets =
    viskores::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);

  viskores::cont::ArrayHandle<viskores::Id> faceConnectivity;
  viskores::cont::ArrayHandle<viskores::Id3> cellFaceId;
  viskores::cont::ArrayHandle<viskores::Int32> uniqueFaces;

  GenerateFaceConnnectivity(cellSetUnstructured,
                            shapes,
                            conn,
                            shapeOffsets,
                            coordinates,
                            faceConnectivity,
                            cellFaceId,
                            BoundingBox,
                            FaceOffsets,
                            uniqueFaces);

  viskores::cont::ArrayHandle<viskores::Id4> triangles;
  //
  //Faces
  triangles = ExtractFaces(cellFaceId, uniqueFaces, shapes, conn, shapeOffsets);

  // scatter the coonectivity into the original order
  viskores::worklet::DispatcherMapField<WriteFaceConn>(WriteFaceConn())
    .Invoke(cellFaceId, this->FaceOffsets, faceConnectivity);

  FaceConnectivity = faceConnectivity;
  Triangles = triangles;

  viskores::Float64 time = timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

struct StructuredTrianglesFunctor
{
  template <typename Device>
  VISKORES_CONT bool operator()(Device,
                                viskores::cont::ArrayHandleCounting<viskores::Id>& counting,
                                viskores::cont::ArrayHandle<viskores::Id4>& triangles,
                                viskores::cont::CellSetStructured<3>& cellSet) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::worklet::DispatcherMapField<StructuredExternalTriangles> dispatch(
      StructuredExternalTriangles(cellSet.PrepareForInput(
        Device(), viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint(), token)));

    dispatch.SetDevice(Device());
    dispatch.Invoke(counting, triangles);
    return true;
  }
};

// Should we just make this name BuildConnectivity?
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Id4> MeshConnectivityBuilder::ExternalTrianglesStructured(
  viskores::cont::CellSetStructured<3>& cellSetStructured)
{
  viskores::cont::Timer timer;
  timer.Start();

  viskores::Id3 cellDims = cellSetStructured.GetCellDimensions();
  viskores::Id numFaces =
    cellDims[0] * cellDims[1] * 2 + cellDims[1] * cellDims[2] * 2 + cellDims[2] * cellDims[0] * 2;

  viskores::cont::ArrayHandle<viskores::Id4> triangles;
  triangles.Allocate(numFaces * 2);
  viskores::cont::ArrayHandleCounting<viskores::Id> counting(0, 1, numFaces);

  viskores::cont::TryExecute(StructuredTrianglesFunctor(), counting, triangles, cellSetStructured);

  viskores::Float64 time = timer.GetElapsedTime();
  Logger::GetInstance()->AddLogData("structured_external_faces", time);

  return triangles;
}

viskores::cont::ArrayHandle<viskores::Id> MeshConnectivityBuilder::GetFaceConnectivity()
{
  return FaceConnectivity;
}

viskores::cont::ArrayHandle<viskores::Id> MeshConnectivityBuilder::GetFaceOffsets()
{
  return FaceOffsets;
}

viskores::cont::ArrayHandle<viskores::Id4> MeshConnectivityBuilder::GetTriangles()
{
  return Triangles;
}

VISKORES_CONT
MeshConnectivityContainer* MeshConnectivityBuilder::BuildConnectivity(
  const viskores::cont::UnknownCellSet& cellset,
  const viskores::cont::CoordinateSystem& coordinates)
{
  enum MeshType
  {
    Unsupported = 0,
    Structured = 1,
    Unstructured = 2,
    UnstructuredSingle = 3,
    RZStructured = 3,
    RZUnstructured = 4
  };

  MeshType type = Unsupported;
  if (cellset.CanConvert<viskores::cont::CellSetExplicit<>>())
  {
    type = Unstructured;
  }

  else if (cellset.CanConvert<viskores::cont::CellSetSingleType<>>())
  {
    viskores::cont::CellSetSingleType<> singleType =
      cellset.AsCellSet<viskores::cont::CellSetSingleType<>>();
    //
    // Now we need to determine what type of cells this holds
    //
    viskores::cont::ArrayHandleConstant<viskores::UInt8> shapes = singleType.GetShapesArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    viskores::UInt8 shapeType = shapes.ReadPortal().Get(0);
    if (shapeType == CELL_SHAPE_HEXAHEDRON)
      type = UnstructuredSingle;
    if (shapeType == CELL_SHAPE_TETRA)
      type = UnstructuredSingle;
    if (shapeType == CELL_SHAPE_WEDGE)
      type = UnstructuredSingle;
    if (shapeType == CELL_SHAPE_PYRAMID)
      type = UnstructuredSingle;
  }
  else if (cellset.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    type = Structured;
  }

  if (type == Unsupported)
  {
    throw viskores::cont::ErrorBadValue("MeshConnectivityBuilder: unsupported cell set type");
  }
  viskores::Bounds coordBounds = coordinates.GetBounds();

  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("mesh_conn_construction");

  MeshConnectivityContainer* meshConn = nullptr;
  viskores::cont::Timer timer;
  timer.Start();

  if (type == Unstructured)
  {
    viskores::cont::CellSetExplicit<> cells =
      cellset.AsCellSet<viskores::cont::CellSetExplicit<>>();
    this->BuildConnectivity(cells, coordinates.GetDataAsMultiplexer(), coordBounds);
    meshConn = new MeshConnectivityContainerUnstructured(
      cells, coordinates, FaceConnectivity, FaceOffsets, Triangles);
  }
  else if (type == UnstructuredSingle)
  {
    viskores::cont::CellSetSingleType<> cells =
      cellset.AsCellSet<viskores::cont::CellSetSingleType<>>();
    this->BuildConnectivity(cells, coordinates.GetDataAsMultiplexer(), coordBounds);
    meshConn =
      new MeshConnectivityContainerSingleType(cells, coordinates, FaceConnectivity, Triangles);
  }
  else if (type == Structured)
  {
    viskores::cont::CellSetStructured<3> cells =
      cellset.AsCellSet<viskores::cont::CellSetStructured<3>>();
    Triangles = this->ExternalTrianglesStructured(cells);
    meshConn = new MeshConnectivityContainerStructured(cells, coordinates, Triangles);
  }
  else
  {
    throw viskores::cont::ErrorBadValue(
      "MeshConnectivityBuilder: unsupported cell set type. Logic error, this should not happen");
  }

  viskores::Float64 time = timer.GetElapsedTime();
  logger->CloseLogEntry(time);
  return meshConn;
}
}
}
} //namespace viskores::rendering::raytracing
