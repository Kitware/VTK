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
// The actual cell types (CellType.hh) supported by vlPolyData are: vlVERTEX,
// vlPOLY_VERTEX, vlLINE, vlPOLYLINE, vlTRIANGLE, vlTRIANGLE_STRIP,
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

  // construct adjacency structure
  void BuildCells();
  void BuildLinks();

  // Special (efficient) operations on poly data. Use carefully.
  void GetPointCells(int ptId, unsigned short& ncells, int* &cells);
  void GetCellEdgeNeighbors(int cellId, int p1, int p2, vlIdList& cellIds);
  void GetCellPoints(int cellId, int& npts, int* &pts);
  int IsTriangle(int v1, int v2, int v3);
  int IsEdge(int v1, int v2);
  int IsPointUsedByCell(int ptId, int cellId);
  void ReplaceCell(int cellId, int npts, int *pts);
  void ReverseCell(int cellId);
  void DeletePoint(int ptId);
  void DeleteCell(int cellId);
  void ReplaceLinkedCell(int cellId, int npts, int *pts);
  void RemoveCellReference(int cellId);
  void ResizeCellList(int ptId, int size);

protected:
  void Initialize();

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

};

// Description:
// Efficient method to obtain cells using a particular point. Make sure that
// routine BuildLinks() has been called.
inline void vlPolyData::GetPointCells(int ptId, unsigned short& ncells, 
                                      int* &cells)
{
  ncells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);
}

// Description:
// Given three vertices, determine whether it's a triangle. Make sure 
// BuildLinks() has been called first.
inline int vlPolyData::IsTriangle(int v1, int v2, int v3)
{
  unsigned short int n1;
  int i, j, n2, *cells, tVerts[3], *tVerts2;

  tVerts[0] = v1;
  tVerts[1] = v2;
  tVerts[2] = v3;

  for (i=0; i<3; i++) 
    {
    this->GetPointCells(tVerts[i], n1, cells);
    for (j=0; j<n1; j++) 
      {
      this->GetCellPoints(cells[j], n2, tVerts2);
      if ( (tVerts[0] == tVerts2[0] || tVerts[0] == tVerts2[1] ||
      tVerts[0] == tVerts2[2]) &&
      (tVerts[1] == tVerts2[0] || tVerts[1] == tVerts2[1] ||
      tVerts[1] == tVerts2[2]) &&
      (tVerts[2] == tVerts2[0] || tVerts[2] == tVerts2[1] ||
      tVerts[2] == tVerts2[2]) )
        return 1;
      }
    }
  return 0;
}

// Description:
// Determine whether a point is used by a particular cell. If it is, return
// non-zero. Make sure BuildCells() has been called first.
inline int vlPolyData::IsPointUsedByCell(int ptId, int cellId)
{
  int npts, *pts;
  this->GetCellPoints(cellId, npts, pts);
  for (int i=0; i < npts; i++)
    if ( pts[i] == ptId ) return 1;

  return 0;
}

// Description:
// Determine whether two points form an edge. If they do, return non-zero.
// Make sure BuildLinks() has been called first.
inline int vlPolyData::IsEdge(int p1, int p2)
{
  unsigned short int ncells;
  int i, *cells;

  this->GetPointCells(p1,ncells,cells);
  for (i=0; i < ncells; i++)
    if ( this->IsPointUsedByCell(p2,cells[i]) ) return 1;

  return 0;
}

inline void vlPolyData::DeletePoint(int ptId)
{
  this->Links->DeletePoint(ptId);
}

inline void vlPolyData::DeleteCell(int cellId)
{
  this->Cells->DeleteCell(cellId);
}

inline void vlPolyData::RemoveCellReference(int cellId)
{
  int npts, *pts;
  this->GetCellPoints(cellId, npts, pts);
  for (int i=0; i<npts; i++)
    this->Links->RemoveCellReference(cellId, pts[i]);  
}

inline void vlPolyData::ResizeCellList(int ptId, int size)
{
  this->Links->ResizeCellList(ptId,size);
}

#endif


