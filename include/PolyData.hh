//
// Represent polygonal data (topological vertices (as compared to
// geometric point coordinates), lines, polygons, and triangle strips
//
#ifndef PolyData_h
#define PolyData_h

#include "DataSet.h"
#include "Params.h"
#include "FPoints.h"
#include "CellArr.h"
#include "CellList.h"
#include "LinkList.h"

#define MAX_VERTS 64

class PolyData : virtual public DataSet {
public:
  /* dataset interface */
  PolyData();
  PolyData(const PolyData& pd);
  ~PolyData();
  virtual int numCells();
  virtual int numPoints();
  int cellDimension(int cellId);
  void cellPoints(int cellId, IdList& ptId);
  virtual void Initialize();
  void pointCoords(IdList& ptId, FloatPoints& fp);

  /* PolyData specific */
  void setPoints (FloatPoints* pts);
  FloatPoints *getPoints();
  void setVerts (CellArray* v);
  CellArray *getVerts();
  void setLines (CellArray* l);
  CellArray *getLines();
  void setPolys (CellArray* p);
  CellArray *getPolys();
  void setStrips (CellArray* s);
  CellArray *getStrips();
  int numVerts();
  int numLines();
  int numPolys();
  int numStrips();

private:
  /* point data (i.e., scalars, vectors, normals, tcoords) inherited */
  FloatPoints *points;
  CellArray *verts;
  CellArray *lines;
  CellArray *polys;
  CellArray *strips;
  CellList cells;
  LinkList links;
  /* trick to simplify traversal */
  static CellArray dummy;

  void build_cells();
  void build_links();
};

#endif


