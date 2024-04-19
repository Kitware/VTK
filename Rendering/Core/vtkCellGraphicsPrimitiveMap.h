// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCellGraphicsPrimitiveMap
 * @brief   Maps cell connectivity and offsets from VTK data model into primitives that graphics
 * libraries expect (points, lines and triangles)
 *
 * When given only vertices, lines and triangles and using 32-bit integer IDs, this class opts into
 * low memory code paths, i.e, does not copy indices into new arrays.
 * When input has poly-vertices, poly-lines and polygons or triangle strips or uses 64-bit integer
 * IDs, this class makes an additional copy of the indices. A message is logged in the console to
 * warn about potential OOM errors.
 */

#ifndef vtkCellGraphicsPrimitiveMap_h
#define vtkCellGraphicsPrimitiveMap_h

#include "vtkObject.h"

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSmartPointer.h"        // For vtkSmartPointer
#include "vtkTypeInt32Array.h"      // For PrimitiveDescriptor::[PrimitiveToCell, VertexIDs]
#include "vtkTypeUInt8Array.h"      // For PrimitiveDescriptor::EdgeArray

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class VTKRENDERINGCORE_EXPORT vtkCellGraphicsPrimitiveMap : vtkObject
{
public:
  static vtkCellGraphicsPrimitiveMap* New();
  vtkTypeMacro(vtkCellGraphicsPrimitiveMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  struct CellTypeMapperOffsets
  {
    vtkIdType CellIdOffset = 0;
    vtkIdType EdgeValueBufferOffset = 0;
    vtkIdType PointIdOffset = 0;
    vtkIdType PrimitiveIdOffset = 0;
    vtkIdType VertexIdOffset = 0;
    friend std::ostream& operator<<(std::ostream& os, const CellTypeMapperOffsets& offsets)
    {
      os << "\nCellIdOffset: " << offsets.CellIdOffset << '\n'
         << "EdgeValueBufferOffset: " << offsets.EdgeValueBufferOffset << '\n'
         << "PointIdOffset: " << offsets.PointIdOffset << '\n'
         << "PrimitiveIdOffset: " << offsets.PrimitiveIdOffset << '\n'
         << "VertexIdOffset: " << offsets.VertexIdOffset << '\n';
      return os;
    }
  };

  struct PrimitiveDescriptor
  {
    vtkSmartPointer<vtkTypeUInt8Array> EdgeArray;
    vtkSmartPointer<vtkTypeInt32Array> PrimitiveToCell;
    vtkSmartPointer<vtkTypeInt32Array> VertexIDs;
    int PrimitiveSize = 0;
    vtkTypeInt32 LocalCellIdOffset = 0;
  };
  /// break down and tag vertices with their vtk cell id.
  static PrimitiveDescriptor ProcessVertices(vtkPolyData* mesh);
  /// break down and tag lines with their vtk cell id.
  static PrimitiveDescriptor ProcessLines(vtkPolyData* mesh);
  /// break down (into triangles) and tag polygons with their vtk cell id. Also generates edge masks
  /// used to hide internal edges of the polygon.
  static PrimitiveDescriptor ProcessPolygons(vtkPolyData* mesh);
  /// break down (into triangles) and tag strips with their vtk cell id.
  static PrimitiveDescriptor ProcessStrips(vtkPolyData* mesh);

protected:
  vtkCellGraphicsPrimitiveMap();
  ~vtkCellGraphicsPrimitiveMap() override;

private:
  vtkCellGraphicsPrimitiveMap(const vtkCellGraphicsPrimitiveMap&) = delete;
  void operator=(const vtkCellGraphicsPrimitiveMap&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
