//
// Represent polygonal data (topological vertices (as compared to
// geometric point coordinates), lines, polygons, and triangle strips
//
#ifndef __vlPolyData_h
#define __vlPolyData_h

#include "DataSet.h"
#include "FPoints.h"
#include "CellArr.h"
#include "CellList.h"
#include "LinkList.h"

#define MAX_VERTS MAX_CELL_SIZE

class vlPolyData : public vlDataSet 
{
public:
  /* dataset interface */
  vlPolyData();
  vlPolyData(const vlPolyData& pd);
  ~vlPolyData();
  int NumCells();
  int NumPoints();
  int CellDimension(int cellId);
  void CellPoints(int cellId, vlIdList& ptId);
  void Initialize();
  void PointCoords(vlIdList& ptId, vlFloatPoints& fp);

  /* PolyData specific */
  void SetPoints (vlFloatPoints* pts);
  vlFloatPoints *GetPoints();
  void SetVerts (vlCellArray* v);
  vlCellArray *GetVerts();
  void SetLines (vlCellArray* l);
  vlCellArray *GetLines();
  void SetPolys (vlCellArray* p);
  vlCellArray *GetPolys();
  void SetStrips (vlCellArray* s);
  vlCellArray *GetStrips();
  int NumVerts();
  int NumLines();
  int NumPolys();
  int NumStrips();

private:
  /* point data (i.e., scalars, vectors, normals, tcoords) inherited */
  vlFloatPoints *Points;
  vlCellArray *Verts;
  vlCellArray *Lines;
  vlCellArray *Polys;
  vlCellArray *Strips;
  vlCellList Cells;
  vlLinkList Links;
  /* trick to simplify traversal */
  static vlCellArray Dummy;

  void BuildCells();
  void BuildLinks();
};

#endif


