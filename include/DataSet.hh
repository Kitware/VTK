//
// Abstract class for specifying dataset behaviour
//
#ifndef __vlDataSet_h
#define __vlDataSet_h

#include "Object.hh"
#include "IdList.hh"
#include "FPoints.hh"
#include "PtData.hh"
#include "Mapper.hh"

#define MAX_CELL_SIZE 128

class vlDataSet : virtual public vlObject 
{
public:
  vlDataSet();
  char *GetClassName() {return "vlDataSet";};
  virtual vlDataSet *CopySelf() = 0;
  virtual int NumCells() = 0;
  virtual int NumPoints() = 0;
  virtual int CellDimension(int cellId) = 0;
  virtual void CellPoints(int cellId, vlIdList& ptId) = 0;
  virtual void Initialize();
  virtual vlFloatTriple& PointCoord(int i) = 0;
  virtual void PointCoords(vlIdList& ptId, vlFloatPoints& fp) = 0;
  virtual void Update() {};

  unsigned long int GetMtime();

  virtual void ComputeBounds();
  float *GetBounds();
  float *GetCenter();
  float GetLength();
  
  vlPointData *GetPointData() {return &this->PointData;};

  virtual vlMapper *MakeMapper(vlDataSet *ds) = 0;

protected:
  vlPointData PointData;   // Scalars, vectors, etc. associated w/ each point
  vlTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];
  vlMapper *Mapper;
};

#endif


