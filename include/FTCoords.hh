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
  vlFloatTCoords() {};
  int Initialize(const int sz, const int dim=2, const int ext=1000) 
    {return this->TC.Initialize(dim*sz,dim*ext);};
  vlFloatTCoords(const vlFloatTCoords& ftc) 
    {this->TC = ftc.TC;this->Dimension = ftc.Dimension;};
  vlFloatTCoords(const int sz, const int d=2, const int ext=1000):Dimension(d), TC(d*sz,d*ext) {};
  ~vlFloatTCoords() {};
  char *GetClassName() {return "vlFloatTCoords";};
  int NumTCoords() {return (this->TC.GetMaxId()+1)/this->Dimension;};
  void Reset() {this->TC.Reset();};
  vlFloatTCoords &operator=(const vlFloatTCoords& ftc);
  void operator+=(const vlFloatTCoords& ftc) {this->TC += ftc.TC;};
  void operator+=(const vlFloatTriple& ft) {
    for(int j=0; j<this->Dimension; j++) this->TC += ft.X[j];
  }
  vlFloatTriple &operator[](const int i) {this->Ft.X = this->TC.GetPtr(3*i); return this->Ft;};
  void InsertTCoord(const int i, vlFloatTriple &ft) {
    for(int j=0; j<this->Dimension; j++) this->TC.InsertValue(3*i+j, ft.X[j]);
  }
  void InsertTCoord(const int i, float *x) {
    for(int j=0; j<this->Dimension; j++) this->TC.InsertValue(3*i+j, x[j]);
  }
  int InsertNextTCoord(float *x) {
    int id = this->TC.InsertNextValue(x[0]);
    for(int j=1; j<this->Dimension; j++) this->TC.InsertNextValue(x[j]);
    return id/this->Dimension;
  }

  vlSetClampMacro(Dimension,int,1,3);
  vlGetMacro(Dimension,int);

  void GetTCoords(vlIdList& ptId, vlFloatTCoords& ftc);
  float *GetArray();
private:
  vlFloatArray TC;
  vlFloatTriple Ft;
  int Dimension;
};

#endif
