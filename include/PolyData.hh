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
#include "CellList.hh"
#include "LinkList.hh"

class vlPolyData : public vlDataSet 
{
public:
  vlPolyData();
  vlPolyData(const vlPolyData& pd);
  ~vlPolyData();
  char *GetClassName() {return "vlPolyData";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject();
  int GetNumberOfPoints();
  int GetNumberOfCells();
  float *GetPoint(int ptId) {return this->Points->GetPoint(ptId);};
  vlCell *GetCell(int cellId);
  vlMapper *MakeMapper();
  void Initialize();

  void ComputeBounds();

  // PolyData specific stuff follows
  vlSetObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

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

  // following stuff supports cell structure
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

  LoadAll() {this->LoadVertsOn(); this->LoadLinesOn(); 
             this->LoadPolysOn(); this->LoadStripsOn();};
  LoadNone() {this->LoadVertsOff(); this->LoadLinesOff(); 
             this->LoadPolysOff(); this->LoadStripsOff();};

  SetReadOnly() {this->SetWritable(0);};
  vlBooleanMacro(Writable,int);
  vlSetMacro(Writable,int)
  vlGetMacro(Writable,int);
  
  vlBooleanMacro(TriangleMesh,int);
  vlSetMacro(TriangleMesh,int);
  vlGetMacro(TriangleMesh,int);

protected:
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vlPoints *Points;
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
  int TriangleMesh;
  int Writable;
  vlCellList *Cells;
  vlLinkList *Links;
  void BuildCells();
  void BuildLinks();

};

#endif


