//
// Abstract interface to 3D normals.
//
#ifndef __vlNormals_h
#define __vlNormals_h

#include "Object.hh"

class vlIdList;
class vlFloatNormals;

class vlNormals : public vlObject 
{
public:
  virtual ~vlNormals() {};
  virtual vlNormals *MakeObject(int sze, int ext=1000) = 0;
  virtual int NumNormals() = 0;
  virtual float *GetNormal(int i) = 0;
  virtual void SetNormal(int i,float x[3]) = 0;     // fast insert
  virtual void InsertNormal(int i, float x[3]) = 0; // allocates memory as necessary
  void GetNormals(vlIdList& ptId, vlFloatNormals& fp);
  char *GetClassName() {return "vlNormals";};
};

#endif
