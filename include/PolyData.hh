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
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlPolyData(*this);};
  int GetNumberOfCells();
  vlCell *GetCell(int cellId);
  int GetCellType(int cellId);
  vlMapper *MakeMapper();
  void Initialize();
  void GetCellPoints(int cellId, vlIdList *ptIds);
  void GetPointCells(int ptId, vlIdList *cellIds);

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

  // create verts, lines, polys, tmeshes from cell object
  void InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE]);

  // following stuff supports cell structure
  // Booleans control whether certain types of data are loaded.
  vlBooleanMacro(LoadVerts,int);
  vlSetMacro(LoadVerts,int);
  vlGetMacro(LoadVerts,int);

  vlBooleanMacro(LoadLines,int);
  vlSetMacro(LoadLines,int);
  vlGetMacro(LoadLines,int);

  vlBooleanMacro(LoadPolys,int);
  vlSetMacro(LoadPolys,int);
  vlGetMacro(LoadPolys,int);

  vlBooleanMacro(LoadStrips,int);
  vlSetMacro(LoadStrips,int);
  vlGetMacro(LoadStrips,int);

  void LoadAll() {this->LoadVertsOn(); this->LoadLinesOn(); 
             this->LoadPolysOn(); this->LoadStripsOn();};
  void LoadNone() {this->LoadVertsOff(); this->LoadLinesOff(); 
             this->LoadPolysOff(); this->LoadStripsOff();};

  void SetReadOnly() {this->SetWritable(0);};
  vlBooleanMacro(Writable,int);
  vlSetMacro(Writable,int)
  vlGetMacro(Writable,int);
  
protected:
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vlCellArray *Verts;
  vlCellArray *Lines;
  vlCellArray *Polys;
  vlCellArray *Strips;
  // dummy static member below used as a trick to simplify traversal
  static vlCellArray *Dummy;
  // supports building Cell structure
  int LoadVerts;
  int LoadLines;
  int LoadPolys;
  int LoadStrips;
  int Writable;
  vlCellList *Cells;
  vlLinkList *Links;
  void BuildCells();
  void BuildLinks();

};

#endif


