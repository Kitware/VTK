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
// .NAME vlPolyData - concrete dataset represents vertices, lines, polygons, and triangle strips
// .SECTION Description
// vlPolyData is a data object that is a concrete implementation of vlDataSet.
// vlPolyData represents a geometric structure consisting of vertices, lines,
// polygons, and triangle strips. Point attribute values (e.g., scalars,
// vectors, etc.) are also represented.
//
// The actual cell types (CellType.hh) supported by vlPolyData are: vlPOINT,
// vlPOLY_POINTS, vlLINE, vlPOLYLINE, vlTRIANGLE, vlTRIANGLE_STRIP,
// vlPOLYGON, vlRECTANGLE, and vlQUAD.

#ifndef __vlPolyData_h
#define __vlPolyData_h

#include "PointSet.hh"
#include "FPoints.hh"
#include "CellArr.hh"
#include "CellList.hh"
#include "LinkList.hh"

class vlPolyData : public vlPointSet 
{
public:
  vlPolyData();
  vlPolyData(const vlPolyData& pd);
  ~vlPolyData();
  char *GetClassName() {return "vlPolyData";};
  char *GetDataType() {return "vlPolyData";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlPolyData(*this);};
  int GetNumberOfCells();
  vlCell *GetCell(int cellId);
  int GetCellType(int cellId);
  void Initialize();
  void GetCellPoints(int cellId, vlIdList& ptIds);
  void GetPointCells(int ptId, vlIdList& cellIds);

  // Can't use macros to set/get following cell arrays.  This is due to tricks
  // required to support traversal methods.
  void SetVerts (vlCellArray* v);
  vlCellArray *GetVerts();

  void SetLines (vlCellArray* l);
  vlCellArray *GetLines();

  void SetPolys (vlCellArray* p);
  vlCellArray *GetPolys();

  void SetStrips (vlCellArray* s);
  vlCellArray *GetStrips();

  int GetNumberOfVerts();
  int GetNumberOfLines();
  int GetNumberOfPolys();
  int GetNumberOfStrips();

  // Allocate storage for cells when using the following InsertNextCell method
  void Allocate(int numCells=1000, int extSize=1000);
  // create verts, lines, polys, tmeshes from cell object
  int InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE]);
  // Use this method to reclaim memory when using InsertNextCell()
  void Squeeze();

  // special operations on cell
  void ReverseCell(int cellId);
  void ReplaceCell(int cellId, vlIdList& ptIds);

protected:
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vlCellArray *Verts;
  vlCellArray *Lines;
  vlCellArray *Polys;
  vlCellArray *Strips;

  // dummy static member below used as a trick to simplify traversal
  static vlCellArray *Dummy;

  // supporting structures for more complex topological operations
  // built only when necessary
  vlCellList *Cells;
  vlLinkList *Links;
  void BuildCells();
  void BuildLinks();

};

#endif


