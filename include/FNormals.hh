//
// Floating point representation of 3D normals.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatNormals_h
#define __vlFloatNormals_h

#include "Object.hh"
#include "FArray.hh"
#include "FTriple.hh"
#include "IdList.hh"

class vlFloatNormals : public vlObject 
{
public:
  vlFloatNormals();
  int Initialize(const int sz, const int ext=1000) {return this->N.Initialize(3*sz,3*ext);};
  vlFloatNormals(const vlFloatNormals& fn);
  vlFloatNormals(const int sz, const int ext=1000);
  vlFloatNormals(vlFloatArray& fa);
  ~vlFloatNormals();
  char *GetClassName() {return "vlFloatNormals";};
  int NumNormals();
  void Reset();
  vlFloatNormals &operator=(const vlFloatNormals& fn);
  void operator+=(const vlFloatNormals& fn);
  void operator+=(const vlFloatTriple& ft) {
    this->N += ft.X[0];
    this->N += ft.X[1];
    this->N += ft.X[2];
  }
  vlFloatTriple &operator[](const int i) {this->Ft.X = this->N.GetPtr(3*i); return this->Ft;};
  void InsertNormal(const int i, vlFloatTriple &ft) {
      this->N.InsertValue(3*i+2, ft.X[2]);
      this->N[3*i] =  ft.X[0];
      this->N[3*i+1] =  ft.X[1];
  }
  void InsertNormal(const int i, float *x) {
      this->N.InsertValue(3*i+2, x[2]);
      this->N[3*i] =  x[0];
      this->N[3*i+1] =  x[1];
  }
  int InsertNextNormal(float *x) {
    int id = this->N.InsertNextValue(x[0]);
    this->N.InsertNextValue(x[1]);
    this->N.InsertNextValue(x[2]);
    return id/3;
  }
  void GetNormals(vlIdList& ptId, vlFloatNormals& fn);
  float *GetArray();
private:
  vlFloatArray N;
  vlFloatTriple Ft;
};

#endif
