/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWebGLPolyData
// .SECTION Description
// PolyData representation for WebGL.

#ifndef vtkWebGLPolyData_h
#define vtkWebGLPolyData_h

#include "vtkWebGLObject.h"
#include "vtkWebGLExporterModule.h" // needed for export macro

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
  void PrintSelf(ostream &os, vtkIndent indent);

  void GenerateBinaryData();
  unsigned char* GetBinaryData(int part);
  int GetBinarySize(int part);
  int GetNumberOfParts();

  void GetPoints(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize);

  void GetLinesFromPolygon(vtkMapper* mapper, vtkActor* actor, int lineMaxSize, double* edgeColor);
  void GetLines(vtkTriangleFilter* polydata, vtkActor* actor, int lineMaxSize);
  void GetColorsFromPolyData(unsigned char* color, vtkPolyData* polydata, vtkActor* actor);

  // Get following data from the actor
  void GetPolygonsFromPointData(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize);
  void GetPolygonsFromCellData(vtkTriangleFilter* polydata, vtkActor* actor, int maxSize);
  void GetColorsFromPointData(unsigned char* color, vtkPointData* pointdata, vtkPolyData* polydata, vtkActor* actor);

  void SetMesh(float* _vertices, int _numberOfVertices, int* _index, int _numberOfIndexes, float* _normals, unsigned char* _colors, float* _tcoords, int maxSize);
  void SetLine(float* _points, int _numberOfPoints, int* _index, int _numberOfIndex, unsigned char* _colors, int maxSize);
  void SetPoints(float* points, int numberOfPoints, unsigned char* colors, int maxSize);
  void SetTransformationMatrix(vtkMatrix4x4* m);

protected:
  vtkWebGLPolyData();
  ~vtkWebGLPolyData();

private:
  vtkWebGLPolyData(const vtkWebGLPolyData&); // Not implemented
  void operator=(const vtkWebGLPolyData&);   // Not implemented

  vtkTriangleFilter* TriangleFilter;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
