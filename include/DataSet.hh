//
// Abstract class for specifying dataset behaviour
//
#ifndef DataSet_h
#define DataSet_h

#include "Params.h"
#include "TimeSt.h"
#include "RefCount.h"
#include "IdList.h"
#include "FPoints.h"
#include "PtData.h"

class DataSet : virtual public TimeStamp, virtual public RefCount {
public:
  DataSet();
  DataSet(const DataSet& ds);
  virtual ~DataSet();
  virtual int numCells() = 0;
  virtual int numPoints() = 0;
  virtual int cellDimension(int cellId) = 0;
  virtual void cellPoints(int cellId, IdList& ptId) = 0;
  virtual void Initialize() = 0;
  virtual void pointCoords(IdList& ptId, FloatPoints& fp);
  virtual void update() {};

  void setPointData (PointData* pd);
  PointData *getPointData();

private:
  PointData *pointData;
};

#endif


