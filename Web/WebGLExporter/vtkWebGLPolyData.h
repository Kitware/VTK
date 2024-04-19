// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGLPolyData
 * @brief   PolyData representation for WebGL.
 */

#ifndef vtkWebGLPolyData_h
#define vtkWebGLPolyData_h

#include "vtkWebGLExporterModule.h" // needed for export macro
#include "vtkWebGLObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkMatrix4x4;
class vtkMapper;
class vtkPointData;
class vtkPolyData;
class vtkTriangleFilter;

class VTKWEBGLEXPORTER_EXPORT vtkWebGLPolyData : public vtkWebGLObject
{
public:
  static vtkWebGLPolyData* New();
  vtkTypeMacro(vtkWebGLPolyData, vtkWebGLObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void GenerateBinaryData() override;
  unsigned char* GetBinaryData(int part) override;
  int GetBinarySize(int part) override;
  int GetNumberOfParts() override;

  void GetPoints(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize);

  void GetLinesFromPolygon(vtkMapper* mapper, vtkActor* actor, int lineMaxSize, double* edgeColor);
  void GetLines(vtkTriangleFilter* polydata, vtkActor* actor, int lineMaxSize);
  void GetColorsFromPolyData(unsigned char* color, vtkPolyData* polydata, vtkActor* actor);

  // Get following data from the actor
  void GetPolygonsFromPointData(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize);
  void GetPolygonsFromCellData(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize);
  void GetColorsFromPointData(
    unsigned char* color, vtkPointData* pointdata, vtkPolyData* polydata, vtkActor* actor);

  void SetMesh(float* _vertices, int _numberOfVertices, int* _index, int _numberOfIndexes,
    float* _normals, unsigned char* _colors, float* _tcoords, int maxSize);
  void SetLine(float* _points, int _numberOfPoints, int* _index, int _numberOfIndex,
    unsigned char* _colors, int maxSize);
  void SetPoints(float* points, int numberOfPoints, unsigned char* colors, int maxSize);
  void SetTransformationMatrix(vtkMatrix4x4* m);

protected:
  vtkWebGLPolyData();
  ~vtkWebGLPolyData() override;

private:
  vtkWebGLPolyData(const vtkWebGLPolyData&) = delete;
  void operator=(const vtkWebGLPolyData&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
