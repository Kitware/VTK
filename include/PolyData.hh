/*=========================================================================

  Program:   Visualization Library
  Module:    PolyData.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Represent polygonal data (topological vertices (as compared to
// geometric point coordinates), lines, polygons, and triangle strips
//
#ifndef __vlPolyData_h
#define __vlPolyData_h

#include "DataSet.hh"
#include "FPoints.hh"
#include "CellArr.hh"

#define MAX_VERTS MAX_CELL_SIZE

class vlPolyData : public vlDataSet 
{
public:
  // dataset interface
  vlPolyData();
  vlPolyData(const vlPolyData& pd);
  ~vlPolyData();
  vlDataSet *MakeObject();
  char *GetClassName() {return "vlPolyData";};
  void PrintSelf(ostream& os, vlIndent indent);
  int NumberOfCells();
  int NumberOfPoints();
  int CellDimension(int cellId);
  void CellPoints(int cellId, vlIdList& ptId);
  void Initialize();
  float *GetPoint(int i) {return this->Points->GetPoint(i);};
  void GetPoints(vlIdList& ptId, vlFloatPoints& fp);
  void ComputeBounds();
  vlMapper *MakeMapper();

  // PolyData specific stuff follows
  vlSetObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

  // Can't use macros to support traversal methods
  void SetVerts (vlCellArray* v);
  vlCellArray *GetVerts();

  void SetLines (vlCellArray* l);
  vlCellArray *GetLines();

  void SetPolys (vlCellArray* p);
  vlCellArray *GetPolys();

  void SetStrips (vlCellArray* s);
  vlCellArray *GetStrips();

  int NumberOfVerts();
  int NumberOfLines();
  int NumberOfPolys();
  int NumberOfStrips();

private:
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vlPoints *Points;
  vlCellArray *Verts;
  vlCellArray *Lines;
  vlCellArray *Polys;
  vlCellArray *Strips;
  // dummy static member below used as a trick to simplify traversal
  static vlCellArray *Dummy;
};

#endif


