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
#ifndef viskores_worklet_internal_TriangulateTables_h
#define viskores_worklet_internal_TriangulateTables_h

#include <viskores/CellShape.h>
#include <viskores/Types.h>

#include <viskores/cont/ExecutionObjectBase.h>

#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace worklet
{
namespace internal
{

using TriangulateArrayHandle =
  viskores::cont::ArrayHandle<viskores::IdComponent, viskores::cont::StorageTagBasic>;

static viskores::IdComponent TriangleCountData[viskores::NUMBER_OF_CELL_SHAPES] = {
  0,  //  0 = viskores::CELL_SHAPE_EMPTY_CELL
  0,  //  1 = viskores::CELL_SHAPE_VERTEX
  0,  //  2 = viskores::CELL_SHAPE_POLY_VERTEX
  0,  //  3 = viskores::CELL_SHAPE_LINE
  0,  //  4 = viskores::CELL_SHAPE_POLY_LINE
  1,  //  5 = viskores::CELL_SHAPE_TRIANGLE
  0,  //  6 = viskores::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = viskores::CELL_SHAPE_POLYGON
  0,  //  8 = viskores::CELL_SHAPE_PIXEL
  2,  //  9 = viskores::CELL_SHAPE_QUAD
  0,  // 10 = viskores::CELL_SHAPE_TETRA
  0,  // 11 = viskores::CELL_SHAPE_VOXEL
  0,  // 12 = viskores::CELL_SHAPE_HEXAHEDRON
  0,  // 13 = viskores::CELL_SHAPE_WEDGE
  0   // 14 = viskores::CELL_SHAPE_PYRAMID
};

static viskores::IdComponent TriangleOffsetData[viskores::NUMBER_OF_CELL_SHAPES] = {
  -1, //  0 = viskores::CELL_SHAPE_EMPTY_CELL
  -1, //  1 = viskores::CELL_SHAPE_VERTEX
  -1, //  2 = viskores::CELL_SHAPE_POLY_VERTEX
  -1, //  3 = viskores::CELL_SHAPE_LINE
  -1, //  4 = viskores::CELL_SHAPE_POLY_LINE
  0,  //  5 = viskores::CELL_SHAPE_TRIANGLE
  -1, //  6 = viskores::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = viskores::CELL_SHAPE_POLYGON
  -1, //  8 = viskores::CELL_SHAPE_PIXEL
  1,  //  9 = viskores::CELL_SHAPE_QUAD
  -1, // 10 = viskores::CELL_SHAPE_TETRA
  -1, // 11 = viskores::CELL_SHAPE_VOXEL
  -1, // 12 = viskores::CELL_SHAPE_HEXAHEDRON
  -1, // 13 = viskores::CELL_SHAPE_WEDGE
  -1  // 14 = viskores::CELL_SHAPE_PYRAMID
};

static viskores::IdComponent TriangleIndexData[] = {
  // viskores::CELL_SHAPE_TRIANGLE
  0,
  1,
  2,
  // viskores::CELL_SHAPE_QUAD
  0,
  1,
  2,
  0,
  2,
  3
};

class TriangulateTablesExecutionObject
{
public:
  using PortalType = TriangulateArrayHandle::ReadPortalType;

  VISKORES_CONT
  TriangulateTablesExecutionObject(const TriangulateArrayHandle& counts,
                                   const TriangulateArrayHandle& offsets,
                                   const TriangulateArrayHandle& indices,
                                   viskores::cont::DeviceAdapterId device,
                                   viskores::cont::Token& token)
    : Counts(counts.PrepareForInput(device, token))
    , Offsets(offsets.PrepareForInput(device, token))
    , Indices(indices.PrepareForInput(device, token))
  {
  }

  template <typename CellShape>
  VISKORES_EXEC viskores::IdComponent GetCount(CellShape shape,
                                               viskores::IdComponent numPoints) const
  {
    if (shape.Id == viskores::CELL_SHAPE_POLYGON)
    {
      return numPoints - 2;
    }
    else
    {
      return this->Counts.Get(shape.Id);
    }
  }

  template <typename CellShape>
  VISKORES_EXEC viskores::IdComponent3 GetIndices(CellShape shape,
                                                  viskores::IdComponent triangleIndex) const
  {
    viskores::IdComponent3 triIndices;
    if (shape.Id == viskores::CELL_SHAPE_POLYGON)
    {
      triIndices[0] = 0;
      triIndices[1] = triangleIndex + 1;
      triIndices[2] = triangleIndex + 2;
    }
    else
    {
      viskores::IdComponent offset = 3 * (this->Offsets.Get(shape.Id) + triangleIndex);
      triIndices[0] = this->Indices.Get(offset + 0);
      triIndices[1] = this->Indices.Get(offset + 1);
      triIndices[2] = this->Indices.Get(offset + 2);
    }
    return triIndices;
  }

private:
  PortalType Counts;
  PortalType Offsets;
  PortalType Indices;
};

class TriangulateTables : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT TriangulateTablesExecutionObject
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return TriangulateTablesExecutionObject(
      this->Counts, this->Offsets, this->Indices, device, token);
  }

  VISKORES_CONT
  TriangulateTables()
    : Counts(viskores::cont::make_ArrayHandle(viskores::worklet::internal::TriangleCountData,
                                              viskores::NUMBER_OF_CELL_SHAPES,
                                              viskores::CopyFlag::Off))
    , Offsets(viskores::cont::make_ArrayHandle(viskores::worklet::internal::TriangleOffsetData,
                                               viskores::NUMBER_OF_CELL_SHAPES,
                                               viskores::CopyFlag::Off))
    , Indices(viskores::cont::make_ArrayHandle(viskores::worklet::internal::TriangleIndexData,
                                               viskores::Id(9),
                                               viskores::CopyFlag::Off))
  {
  }

private:
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};

static viskores::IdComponent TetrahedronCountData[viskores::NUMBER_OF_CELL_SHAPES] = {
  0, //  0 = viskores::CELL_SHAPE_EMPTY_CELL
  0, //  1 = viskores::CELL_SHAPE_VERTEX
  0, //  2 = viskores::CELL_SHAPE_POLY_VERTEX
  0, //  3 = viskores::CELL_SHAPE_LINE
  0, //  4 = viskores::CELL_SHAPE_POLY_LINE
  0, //  5 = viskores::CELL_SHAPE_TRIANGLE
  0, //  6 = viskores::CELL_SHAPE_TRIANGLE_STRIP
  0, //  7 = viskores::CELL_SHAPE_POLYGON
  0, //  8 = viskores::CELL_SHAPE_PIXEL
  0, //  9 = viskores::CELL_SHAPE_QUAD
  1, // 10 = viskores::CELL_SHAPE_TETRA
  0, // 11 = viskores::CELL_SHAPE_VOXEL
  5, // 12 = viskores::CELL_SHAPE_HEXAHEDRON
  3, // 13 = viskores::CELL_SHAPE_WEDGE
  2  // 14 = viskores::CELL_SHAPE_PYRAMID
};

static viskores::IdComponent TetrahedronOffsetData[viskores::NUMBER_OF_CELL_SHAPES] = {
  -1, //  0 = viskores::CELL_SHAPE_EMPTY_CELL
  -1, //  1 = viskores::CELL_SHAPE_VERTEX
  -1, //  2 = viskores::CELL_SHAPE_POLY_VERTEX
  -1, //  3 = viskores::CELL_SHAPE_LINE
  -1, //  4 = viskores::CELL_SHAPE_POLY_LINE
  -1, //  5 = viskores::CELL_SHAPE_TRIANGLE
  -1, //  6 = viskores::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = viskores::CELL_SHAPE_POLYGON
  -1, //  8 = viskores::CELL_SHAPE_PIXEL
  -1, //  9 = viskores::CELL_SHAPE_QUAD
  0,  // 10 = viskores::CELL_SHAPE_TETRA
  -1, // 11 = viskores::CELL_SHAPE_VOXEL
  1,  // 12 = viskores::CELL_SHAPE_HEXAHEDRON
  6,  // 13 = viskores::CELL_SHAPE_WEDGE
  9   // 14 = viskores::CELL_SHAPE_PYRAMID
};

static viskores::IdComponent TetrahedronIndexData[] = {
  // viskores::CELL_SHAPE_TETRA
  0,
  1,
  2,
  3,
  // viskores::CELL_SHAPE_HEXAHEDRON
  0,
  1,
  3,
  4,
  1,
  4,
  5,
  6,
  1,
  4,
  6,
  3,
  1,
  3,
  6,
  2,
  3,
  6,
  7,
  4,
  // viskores::CELL_SHAPE_WEDGE
  0,
  1,
  2,
  4,
  3,
  4,
  5,
  2,
  0,
  2,
  3,
  4,
  // viskores::CELL_SHAPE_PYRAMID
  0,
  1,
  2,
  4,
  0,
  2,
  3,
  4
};

class TetrahedralizeTablesExecutionObject
{
public:
  using PortalType = typename TriangulateArrayHandle::ReadPortalType;
  template <typename Device>
  VISKORES_CONT TetrahedralizeTablesExecutionObject PrepareForExecution(Device) const
  {
    return *this;
  }

  VISKORES_CONT
  TetrahedralizeTablesExecutionObject(const TriangulateArrayHandle& counts,
                                      const TriangulateArrayHandle& offsets,
                                      const TriangulateArrayHandle& indices,
                                      viskores::cont::DeviceAdapterId device,
                                      viskores::cont::Token& token)
    : Counts(counts.PrepareForInput(device, token))
    , Offsets(offsets.PrepareForInput(device, token))
    , Indices(indices.PrepareForInput(device, token))
  {
  }

  template <typename CellShape>
  VISKORES_EXEC viskores::IdComponent GetCount(CellShape shape) const
  {
    return this->Counts.Get(shape.Id);
  }

  template <typename CellShape>
  VISKORES_EXEC viskores::IdComponent4 GetIndices(CellShape shape,
                                                  viskores::IdComponent tetrahedronIndex) const
  {
    viskores::IdComponent4 tetIndices;
    viskores::IdComponent offset = 4 * (this->Offsets.Get(shape.Id) + tetrahedronIndex);
    tetIndices[0] = this->Indices.Get(offset + 0);
    tetIndices[1] = this->Indices.Get(offset + 1);
    tetIndices[2] = this->Indices.Get(offset + 2);
    tetIndices[3] = this->Indices.Get(offset + 3);
    return tetIndices;
  }

private:
  PortalType Counts;
  PortalType Offsets;
  PortalType Indices;
};

class TetrahedralizeTables : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT
  TetrahedralizeTables()
    : Counts(viskores::cont::make_ArrayHandle(viskores::worklet::internal::TetrahedronCountData,
                                              viskores::NUMBER_OF_CELL_SHAPES,
                                              viskores::CopyFlag::Off))
    , Offsets(viskores::cont::make_ArrayHandle(viskores::worklet::internal::TetrahedronOffsetData,
                                               viskores::NUMBER_OF_CELL_SHAPES,
                                               viskores::CopyFlag::Off))
    , Indices(viskores::cont::make_ArrayHandle(viskores::worklet::internal::TetrahedronIndexData,
                                               viskores::Id(44),
                                               viskores::CopyFlag::Off))
  {
  }

  VISKORES_CONT TetrahedralizeTablesExecutionObject
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return TetrahedralizeTablesExecutionObject(
      this->Counts, this->Offsets, this->Indices, device, token);
  }

private:
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};

}
}
}

#endif //viskores_worklet_internal_TriangulateTables_h
