//
// Class for manipulating data associated with points
//
#ifndef __vlPointData_h
#define __vlPointData_h

#include "Object.hh"
#include "FScalars.hh"
#include "FVectors.hh"
#include "FNormals.hh"
#include "FTCoords.hh"

class vlPointData : public vlObject 
{
public:
  vlPointData() : Scalars(0), Vectors(0), Normals(0), TCoords(0) {};
  void Initialize(vlPointData* const pd=0,const int sze=0,const int ext=1000);
  ~vlPointData();
  char *GetClassName() {return "vlPointData";};
  vlPointData (const vlPointData& pd);
  vlPointData &operator=(const vlPointData& pd);
  virtual void Update() {};
  void CopyData(const vlPointData *const from_pd, int from_id, int to_id);

  vlSetObjectMacro (Scalars, vlFloatScalars);
  vlGetObjectMacro (Scalars, vlFloatScalars);

  vlSetObjectMacro (Vectors, vlFloatVectors);
  vlGetObjectMacro (Vectors, vlFloatVectors);

  vlSetObjectMacro (Normals, vlFloatNormals);
  vlGetObjectMacro (Normals, vlFloatNormals);

  vlSetObjectMacro (TCoords, vlFloatTCoords);
  vlGetObjectMacro (TCoords, vlFloatTCoords);

protected:
  vlFloatScalars *Scalars;
  vlFloatVectors *Vectors;
  vlFloatNormals *Normals;
  vlFloatTCoords *TCoords;
};

#endif


