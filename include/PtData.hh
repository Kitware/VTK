//
// Class for manipulating data associated with points
//
#ifndef __vlPointData_h
#define __vlPointData_h

#include "Object.hh"
#include "Scalars.hh"
#include "Vectors.hh"
#include "Normals.hh"
#include "TCoords.hh"

class vlPointData : public vlObject 
{
public:
  vlPointData() : Scalars(0), Vectors(0), Normals(0), TCoords(0) {};
  void Initialize();
  void CopyInitialize(vlPointData* pd, int sze=0, int ext=1000);
  ~vlPointData();
  char *GetClassName() {return "vlPointData";};
  vlPointData (const vlPointData& pd);
  vlPointData &operator=(vlPointData& pd);
  virtual void Update() {};
  void CopyData(vlPointData *from_pd, int from_id, int to_id);

  vlSetObjectMacro (Scalars, vlScalars);
  vlGetObjectMacro (Scalars, vlScalars);

  vlSetObjectMacro (Vectors, vlVectors);
  vlGetObjectMacro (Vectors, vlVectors);

  vlSetObjectMacro (Normals, vlNormals);
  vlGetObjectMacro (Normals, vlNormals);

  vlSetObjectMacro (TCoords, vlTCoords);
  vlGetObjectMacro (TCoords, vlTCoords);

protected:
  vlScalars *Scalars;
  vlVectors *Vectors;
  vlNormals *Normals;
  vlTCoords *TCoords;
};

#endif


