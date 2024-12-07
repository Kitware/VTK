// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLCellToVTKCellMap
 * @brief   OpenGL rendering utility functions
 *
 * vtkOpenGLCellToVTKCellMap provides functions map from opengl primitive ID to vtk
 *
 *
 */

#ifndef vtkOpenGLCellToVTKCellMap_h
#define vtkOpenGLCellToVTKCellMap_h

#include "vtkNew.h" // for ivars
#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkStateStorage.h"           // used for ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkPoints;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLCellToVTKCellMap : public vtkObject
{
public:
  static vtkOpenGLCellToVTKCellMap* New();
  vtkTypeMacro(vtkOpenGLCellToVTKCellMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Create supporting arrays that are needed when rendering cell data
  // Some VTK cells have to be broken into smaller cells for OpenGL
  // When we have cell data we have to map cell attributes from the VTK
  // cell number to the actual OpenGL cell
  //
  // The same concept applies to cell based picking
  //
  void BuildCellSupportArrays(vtkCellArray* [4], int representation, vtkPoints* points);

  void BuildPrimitiveOffsetsIfNeeded(vtkCellArray* [4], int representation, vtkPoints* points);

  vtkIdType ConvertOpenGLCellIdToVTKCellId(bool pointPicking, vtkIdType openGLId);

  // rebuilds if needed
  void Update(vtkCellArray** prims, int representation, vtkPoints* points);

  size_t GetSize() { return this->CellCellMap.size(); }

  vtkIdType* GetPrimitiveOffsets() { return this->PrimitiveOffsets; }

  vtkIdType GetValue(size_t i) { return this->CellCellMap[i]; }

  // what offset should verts start at
  void SetStartOffset(vtkIdType start);

  vtkIdType GetFinalOffset() { return this->PrimitiveOffsets[3] + this->CellMapSizes[3]; }

protected:
  vtkOpenGLCellToVTKCellMap();
  ~vtkOpenGLCellToVTKCellMap() override;

  std::vector<vtkIdType> CellCellMap;
  vtkIdType CellMapSizes[4];
  vtkIdType PrimitiveOffsets[4];
  int BuildRepresentation;
  int StartOffset = 0;
  vtkStateStorage MapBuildState;
  vtkStateStorage TempState;

private:
  vtkOpenGLCellToVTKCellMap(const vtkOpenGLCellToVTKCellMap&) = delete;
  void operator=(const vtkOpenGLCellToVTKCellMap&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
