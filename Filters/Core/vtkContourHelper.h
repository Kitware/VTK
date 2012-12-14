/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourHelper - A utility class used by various contour filters
// .SECTION Description
//  This is a simple utility class that can be used by various contour filters to
//  produce either triangles or polygons based on the outputTriangles parameter
// .SECTION See Also
// vtkContourGrid vtkCutter vtkContourFilter

#ifndef __vtkContourHelper_h
#define __vtkContourHelper_h

#include "vtkSmartPointer.h" //for a member variable
#include "vtkPolygonBuilder.h" //for a member variable

class vtkIncrementalPointLocator;
class vtkCellArray;
class vtkPointData;
class vtkCellData;
class vtkCell;
class vtkDataArray;
class vtkIdList;

class vtkContourHelper
{
public:
  vtkContourHelper(vtkIncrementalPointLocator *locator,
                 vtkCellArray *verts,
                 vtkCellArray *lines,
                 vtkCellArray* polys,
                 vtkPointData *inPd,
                 vtkCellData *inCd,
                 vtkPointData* outPd,
                 vtkCellData *outCd,
                 int estimatedSize,
                 bool outputTriangles);
  ~vtkContourHelper();
  void Contour(vtkCell* cell, double value, vtkDataArray *cellScalars, vtkIdType cellId);

 private:
  vtkIncrementalPointLocator* Locator;
  vtkCellArray* Verts;
  vtkCellArray* Lines;
  vtkCellArray* Polys;
  vtkPointData* InPd;
  vtkCellData* InCd;
  vtkPointData* OutPd;
  vtkCellData* OutCd;
  vtkSmartPointer<vtkCellData> TriOutCd;

  vtkCellArray* Tris;
  vtkPolygonBuilder PolyBuilder;
  vtkIdList* Poly;
  bool GenerateTriangles;
};

#endif
// VTK-HeaderTest-Exclude: vtkContourHelper.h
