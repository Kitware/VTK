//
// Abstract interface to 3D points.
//
#ifndef __vlPoints_h
#define __vlPoints_h

#include "Object.hh"

class vlFloatPoints;
class vlIdList;

class vlPoints : public vlObject 
{
public:
  virtual ~vlPoints() {};
  virtual vlPoints *MakeObject(int sze, int ext=1000) = 0;
  virtual int NumPoints() = 0;
  virtual float *GetPoint(int i) = 0;
  virtual void SetPoint(int i,float x[3]) = 0;       // fast insert
  virtual void InsertPoint(int i, float x[3]) = 0;   // allocates memory as necessary
  void GetPoints(vlIdList& ptId, vlFloatPoints& fp);
  char *GetClassName() {return "vlPoints";};
};

#endif
