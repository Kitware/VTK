//
// Abstract class for specifying dataset behaviour
//
#ifndef __vlDataSet_h
#define __vlDataSet_h

#include "Object.h"
#include "IdList.h"
#include "FPoints.h"
#include "PtData.h"

#define MAX_CELL_SIZE 128

class vlDataSet : virtual public vlObject {
public:
  vlDataSet();
  vlDataSet(const vlDataSet& ds);
  virtual ~vlDataSet();
  virtual int NumCells() = 0;
  virtual int NumPoints() = 0;
  virtual int CellDimension(int cellId) = 0;
  virtual void CellPoints(int cellId, vlIdList& ptId) = 0;
  virtual void Initialize() = 0;
  virtual void PointCoords(vlIdList& ptId, vlFloatPoints& fp) = 0;
  virtual void Update() {};

  void SetPointData (vlPointData* pd);
  vlPointData *GetPointData();

private:
  vlPointData *PointData;
};

#endif


