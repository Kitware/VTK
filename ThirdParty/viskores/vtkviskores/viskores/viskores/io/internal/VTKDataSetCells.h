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
#ifndef viskores_io_internal_VTKDataSetCells_h
#define viskores_io_internal_VTKDataSetCells_h

#include <viskores/CellShape.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/io/ErrorIO.h>

#include <algorithm>
#include <vector>

namespace viskores
{
namespace io
{
namespace internal
{

enum UnsupportedVTKCells
{
  CELL_SHAPE_POLY_VERTEX = 2,
  CELL_SHAPE_POLY_LINE = 4,
  CELL_SHAPE_TRIANGLE_STRIP = 6,
  CELL_SHAPE_PIXEL = 8,
  CELL_SHAPE_VOXEL = 11
};

inline void FixupCellSet(viskores::cont::ArrayHandle<viskores::Id>& connectivity,
                         viskores::cont::ArrayHandle<viskores::IdComponent>& numIndices,
                         viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                         viskores::cont::ArrayHandle<viskores::Id>& permutation)
{
  std::vector<viskores::Id> newConnectivity;
  std::vector<viskores::IdComponent> newNumIndices;
  std::vector<viskores::UInt8> newShapes;
  std::vector<viskores::Id> permutationVec;

  viskores::Id connIdx = 0;
  auto shapesPortal = shapes.ReadPortal();
  auto indicesPortal = numIndices.ReadPortal();
  auto connPortal = connectivity.ReadPortal();
  for (viskores::Id i = 0; i < shapes.GetNumberOfValues(); ++i)
  {
    viskores::UInt8 shape = shapesPortal.Get(i);
    viskores::IdComponent numInds = indicesPortal.Get(i);
    switch (shape)
    {
      case viskores::CELL_SHAPE_VERTEX:
      case viskores::CELL_SHAPE_LINE:
      case viskores::CELL_SHAPE_TRIANGLE:
      case viskores::CELL_SHAPE_QUAD:
      case viskores::CELL_SHAPE_TETRA:
      case viskores::CELL_SHAPE_HEXAHEDRON:
      case viskores::CELL_SHAPE_WEDGE:
      case viskores::CELL_SHAPE_PYRAMID:
      {
        newShapes.push_back(shape);
        newNumIndices.push_back(numInds);
        for (viskores::IdComponent j = 0; j < numInds; ++j)
        {
          newConnectivity.push_back(connPortal.Get(connIdx++));
        }
        permutationVec.push_back(i);
        break;
      }
      case viskores::CELL_SHAPE_POLYGON:
      {
        viskores::IdComponent numVerts = numInds;
        viskores::UInt8 newShape = viskores::CELL_SHAPE_POLYGON;
        if (numVerts == 3)
        {
          newShape = viskores::CELL_SHAPE_TRIANGLE;
        }
        else if (numVerts == 4)
        {
          newShape = viskores::CELL_SHAPE_QUAD;
        }
        newShapes.push_back(newShape);
        newNumIndices.push_back(numVerts);
        for (viskores::IdComponent j = 0; j < numVerts; ++j)
        {
          newConnectivity.push_back(connPortal.Get(connIdx++));
        }
        permutationVec.push_back(i);
        break;
      }
      case CELL_SHAPE_POLY_VERTEX:
      {
        viskores::IdComponent numVerts = numInds;
        for (viskores::IdComponent j = 0; j < numVerts; ++j)
        {
          newShapes.push_back(viskores::CELL_SHAPE_VERTEX);
          newNumIndices.push_back(1);
          newConnectivity.push_back(connPortal.Get(connIdx));
          permutationVec.push_back(i);
          ++connIdx;
        }
        break;
      }
      case CELL_SHAPE_POLY_LINE:
      {
        viskores::IdComponent numLines = numInds - 1;
        for (viskores::IdComponent j = 0; j < numLines; ++j)
        {
          newShapes.push_back(viskores::CELL_SHAPE_LINE);
          newNumIndices.push_back(2);
          newConnectivity.push_back(connPortal.Get(connIdx));
          newConnectivity.push_back(connPortal.Get(connIdx + 1));
          permutationVec.push_back(i);
          ++connIdx;
        }
        connIdx += 1;
        break;
      }
      case CELL_SHAPE_TRIANGLE_STRIP:
      {
        viskores::IdComponent numTris = numInds - 2;
        for (viskores::IdComponent j = 0; j < numTris; ++j)
        {
          newShapes.push_back(viskores::CELL_SHAPE_TRIANGLE);
          newNumIndices.push_back(3);
          if (j % 2)
          {
            newConnectivity.push_back(connPortal.Get(connIdx));
            newConnectivity.push_back(connPortal.Get(connIdx + 1));
            newConnectivity.push_back(connPortal.Get(connIdx + 2));
          }
          else
          {
            newConnectivity.push_back(connPortal.Get(connIdx + 2));
            newConnectivity.push_back(connPortal.Get(connIdx + 1));
            newConnectivity.push_back(connPortal.Get(connIdx));
          }
          permutationVec.push_back(i);
          ++connIdx;
        }
        connIdx += 2;
        break;
      }
      case CELL_SHAPE_PIXEL:
      {
        newShapes.push_back(viskores::CELL_SHAPE_QUAD);
        newNumIndices.push_back(numInds);
        newConnectivity.push_back(connPortal.Get(connIdx + 0));
        newConnectivity.push_back(connPortal.Get(connIdx + 1));
        newConnectivity.push_back(connPortal.Get(connIdx + 3));
        newConnectivity.push_back(connPortal.Get(connIdx + 2));
        permutationVec.push_back(i);
        connIdx += 4;
        break;
      }
      case CELL_SHAPE_VOXEL:
      {
        newShapes.push_back(viskores::CELL_SHAPE_HEXAHEDRON);
        newNumIndices.push_back(numInds);
        newConnectivity.push_back(connPortal.Get(connIdx + 0));
        newConnectivity.push_back(connPortal.Get(connIdx + 1));
        newConnectivity.push_back(connPortal.Get(connIdx + 3));
        newConnectivity.push_back(connPortal.Get(connIdx + 2));
        newConnectivity.push_back(connPortal.Get(connIdx + 4));
        newConnectivity.push_back(connPortal.Get(connIdx + 5));
        newConnectivity.push_back(connPortal.Get(connIdx + 7));
        newConnectivity.push_back(connPortal.Get(connIdx + 6));
        permutationVec.push_back(i);
        connIdx += 8;
        break;
      }
      default:
      {
        throw viskores::io::ErrorIO("Encountered unsupported cell type");
      }
    }
  }

  if (newShapes.size() == static_cast<std::size_t>(shapes.GetNumberOfValues()))
  {
    permutationVec.clear();
  }
  else
  {
    permutation.Allocate(static_cast<viskores::Id>(permutationVec.size()));
    std::copy(permutationVec.begin(),
              permutationVec.end(),
              viskores::cont::ArrayPortalToIteratorBegin(permutation.WritePortal()));
  }

  shapes.Allocate(static_cast<viskores::Id>(newShapes.size()));
  std::copy(newShapes.begin(),
            newShapes.end(),
            viskores::cont::ArrayPortalToIteratorBegin(shapes.WritePortal()));
  numIndices.Allocate(static_cast<viskores::Id>(newNumIndices.size()));
  std::copy(newNumIndices.begin(),
            newNumIndices.end(),
            viskores::cont::ArrayPortalToIteratorBegin(numIndices.WritePortal()));
  connectivity.Allocate(static_cast<viskores::Id>(newConnectivity.size()));
  std::copy(newConnectivity.begin(),
            newConnectivity.end(),
            viskores::cont::ArrayPortalToIteratorBegin(connectivity.WritePortal()));
}

inline bool IsSingleShape(const viskores::cont::ArrayHandle<viskores::UInt8>& shapes)
{
  if (shapes.GetNumberOfValues() < 1)
  {
    // If the data has no cells, is it single shape? That would make sense, but having
    // a single shape cell set requires you to slect a shape, and there are no cells to
    // make that selection from. We could get around that, but it's easier just to treat
    // it as a general explicit grid.
    return false;
  }

  auto shapesPortal = shapes.ReadPortal();
  viskores::UInt8 shape0 = shapesPortal.Get(0);
  for (viskores::Id i = 1; i < shapes.GetNumberOfValues(); ++i)
  {
    if (shapesPortal.Get(i) != shape0)
      return false;
  }

  return true;
}
}
}
} // viskores::io::internal

#endif // viskores_io_internal_VTKDataSetCells_h
