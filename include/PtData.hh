//
// Class for manipulating data associated with points
//
#ifndef PointData_h
#define PointData_h

#include "Params.h"
#include "TimeSt.h"
#include "RefCount.h"
#include "FScalars.h"
#include "FVectors.h"
#include "FNormals.h"
#include "FTCoords.h"

class PointData : public TimeStamp, public RefCount {
public:
  PointData() : scalars(0), vectors(0), normals(0), tcoords(0) {};
  void Initialize(PointData* const pd=0, const int sze=0, const int ext=1000);
  ~PointData();
  PointData::PointData (const PointData& pd);
  virtual void update() {};
  void copyData(const PointData *const from_pd, const int from_id, 
                const PointData* to_pd, const int to_id);

  void setScalars (FloatScalars* s);
  FloatScalars *getScalars() {return scalars;};
  void setVectors (FloatVectors* v);
  FloatVectors *getVectors() {return vectors;};
  void setNormals (FloatNormals* n);
  FloatNormals *getNormals() {return normals;};
  void setTCoords (FloatTCoords* t);
  FloatTCoords *getTCoords() {return tcoords;};

private:
  FloatScalars *scalars;
  FloatVectors *vectors;
  FloatNormals *normals;
  FloatTCoords *tcoords;
};

#endif


