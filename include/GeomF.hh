/*=========================================================================

  Program:   Visualization Library
  Module:    GeomF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Extracts geometry from arbitrary input
//
#ifndef __vlGeometryFilter_h
#define __vlGeometryFilter_h

#include "DS2PolyF.hh"

class vlGeometryFilter : public vlDataSetToPolyFilter
{
public:
  vlGeometryFilter();
  ~vlGeometryFilter() {};
  char *GetClassName() {return "vlGeometryFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(PointMinimum,int,0,LARGE_INTEGER);
  vlGetMacro(PointMinimum,int);

  vlSetClampMacro(PointMaximum,int,0,LARGE_INTEGER);
  vlGetMacro(PointMaximum,int);

  vlSetClampMacro(CellMinimum,int,0,LARGE_INTEGER);
  vlGetMacro(CellMinimum,int);

  vlSetClampMacro(CellMaximum,int,0,LARGE_INTEGER);
  vlGetMacro(CellMaximum,int);

  void SetExtent(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
  void SetExtent(float *extent);
  float *GetExtent() { return this->Extent;};

  vlSetMacro(PointClipping,int);
  vlGetMacro(PointClipping,int);
  vlBooleanMacro(PointClipping,int);

  vlSetMacro(CellClipping,int);
  vlGetMacro(CellClipping,int);
  vlBooleanMacro(CellClipping,int);

  vlSetMacro(ExtentClipping,int);
  vlGetMacro(ExtentClipping,int);
  vlBooleanMacro(ExtentClipping,int);

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


