//
// Abstract class for specifying dataset behaviour
//
#ifndef __vlDataSet_h
#define __vlDataSet_h

#include "Object.hh"
#include "IdList.hh"
#include "FPoints.hh"
#include "PtData.hh"

#define MAX_CELL_SIZE 128

class vlDataSet : virtual public vlObject 
{
public:
  vlDataSet();
  vlDataSet(const vlDataSet& ds);
  ~vlDataSet();
  char *GetClassName() {return "vlDataSet";};
  virtual int NumCells() = 0;
  virtual int NumPoints() = 0;
  virtual int CellDimension(int cellId) = 0;
  virtual void CellPoints(int cellId, vlIdList& ptId) = 0;
  virtual void Initialize() = 0;
  virtual void PointCoords(vlIdList& ptId, vlFloatPoints& fp) = 0;
  virtual void Update() {};
  void GetBounds(float bounds[6]);
  void GetCenter(float center[3]);
  float GetLength();
  
  void SetPointData (vlPointData* pd);
  vlPointData *GetPointData();

protected:
  vlPointData *PointData;  // Scalars, vectors, etc. associated w/ each point
  vlTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];
  virtual void ComputeBounds();
};

#endif


