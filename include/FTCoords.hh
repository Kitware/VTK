//
// Floating point representation of 3D TCoords.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatTCoords_h
#define __vlFloatTCoords_h

#include "Object.hh"
#include "FArray.hh"
#include "FTriple.hh"
#include "IdList.hh"

class vlFloatTCoords : public vlObject 
{
public:
  vlFloatTCoords();
  int Initialize(const int sz, const int dim=2, const int ext=1000) {return this->TC.Initialize(dim*sz,dim*ext);};
  vlFloatTCoords(const vlFloatTCoords& tc);
  vlFloatTCoords(const int sz, const int d=2, const int ext=1000);
  vlFloatTCoords(vlFloatArray& fa);
  ~vlFloatTCoords();
  char *GetClassName() {return "vlFloatTCoords";};
  int NumTCoords();
  void Reset();
  vlFloatTCoords &operator=(const vlFloatTCoords& ftc);
  void operator+=(const vlFloatTCoords& ftc);
  void operator+=(const vlFloatTriple& ft) {
    for(int j=0; j<this->Dim; j++) this->TC += ft.X[j];
  }
  vlFloatTriple &operator[](const int i) {this->Ft.X = this->TC.GetPtr(3*i); return this->Ft;};
  void InsertTCoord(const int i, vlFloatTriple &ft) {
    for(int j=0; j<this->Dim; j++) this->TC.InsertValue(3*i+j, ft.X[j]);
  }
  void InsertTCoord(const int i, float *x) {
    for(int j=0; j<this->Dim; j++) this->TC.InsertValue(3*i+j, x[j]);
  }
  int InsertNextTCoord(float *x) {
    int id = this->TC.InsertNextValue(x[0]);
    for(int j=1; j<this->Dim; j++) this->TC.InsertNextValue(x[j]);
    return id/this->Dim;
  }
  vlFloatTCoords&SetDimension(const int i) {
    this->Dim = ( (i<1) ? 1 : (i>3) ? 3 : i);
    return *this;
  }
  int GetDimension() {return this->Dim;};
  void GetTCoords(vlIdList& ptId, vlFloatTCoords& ftc);
  float *GetArray();
private:
  vlFloatArray TC;
  vlFloatTriple Ft;
  int Dim;
};

#endif
