/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GeomF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkGeometryFilter - extract geometry from data (or convert data to polygonal type)
// .SECTION Description
// vtkGeometryFilter is a general-purpose filter to extract geometry (and 
// associated data) from any type of dataset. Geometry is obtained as 
// follows: all 0D, 1D, and 2D cells are extracted. All 2D faces that are 
// used by only one 3D cell (i.e., boundary faces) are extracted. It is 
// also possible to specify conditions on point ids, cell ids, and on 
// bounding box (referred to as "Extent") to control the extraction process.
//    This filter may be also used to convert any type of data to polygonal
// type. The conversion process may be less than satisfactory for some 3D
// datasets. For example, this filter will extract the outer surface of a 
// volume or structured grid dataset. (For structured data you may want to
// use vtkStructuredPointsGeometryFilter or vtkStructuredGridGeometryFilter).
// .SECTION Caveats
// When vtkGeometryFilter extracts cells (or boundaries of cells) it may create
// duplicate points. Use vtkCleanPolyData to merge duplicate points.
// .SECTION See Also
// vtkStructuredPointsGeometryFilter, vtkStructuredGridGeometryFilter

#ifndef __vtkGeometryFilter_h
#define __vtkGeometryFilter_h

#include "DS2PolyF.hh"

class vtkGeometryFilter : public vtkDataSetToPolyFilter
{
public:
  vtkGeometryFilter();
  ~vtkGeometryFilter() {};
  char *GetClassName() {return "vtkGeometryFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off selection of geometry by point id.
  vtkSetMacro(PointClipping,int);
  vtkGetMacro(PointClipping,int);
  vtkBooleanMacro(PointClipping,int);

  // Description:
  // Turn on/off selection of geometry by cell id.
  vtkSetMacro(CellClipping,int);
  vtkGetMacro(CellClipping,int);
  vtkBooleanMacro(CellClipping,int);

  // Description:
  // Turn on/off selection of geometry via bounding box.
  vtkSetMacro(ExtentClipping,int);
  vtkGetMacro(ExtentClipping,int);
  vtkBooleanMacro(ExtentClipping,int);

  // Description:
  // Specify the minimum point id for point id selection.
  vtkSetClampMacro(PointMinimum,int,0,LARGE_INTEGER);
  vtkGetMacro(PointMinimum,int);

  // Description:
  // Specify the maximum point id for point id selection.
  vtkSetClampMacro(PointMaximum,int,0,LARGE_INTEGER);
  vtkGetMacro(PointMaximum,int);

  // Description:
  // Specify the minimum cell id for point id selection.
  vtkSetClampMacro(CellMinimum,int,0,LARGE_INTEGER);
  vtkGetMacro(CellMinimum,int);

  // Description:
  // Specify the maximum cell id for point id selection.
  vtkSetClampMacro(CellMaximum,int,0,LARGE_INTEGER);
  vtkGetMacro(CellMaximum,int);

  void SetExtent(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
  void SetExtent(float *extent);
  float *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int PointMinimum;
  int PointMaximum;
  int CellMinimum;
  int CellMaximum;
  float Extent[6];
  int PointClipping;
  int CellClipping;
  int ExtentClipping;

};

#endif


