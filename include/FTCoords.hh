/*=========================================================================

  Program:   Visualization Library
  Module:    FTCoords.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFloatTCoords - floating point representation of 3D texture coordinates
// .SECTION Description
// vlFloatTCoords is a concrete implementation of vlTCoords. Texture
// coordinates are represented using float values.

#ifndef __vlFloatTCoords_h
#define __vlFloatTCoords_h

#include "TCoords.hh"
#include "FArray.hh"

class vlFloatTCoords : public vlTCoords
{
public:
  vlFloatTCoords() {};
  vlFloatTCoords(const vlFloatTCoords& ftc) {this->TC = ftc.TC;this->Dimension = ftc.Dimension;};
  vlFloatTCoords(int sz, int d=2, int ext=1000):TC(d*sz,d*ext) {this->Dimension=d;};
  ~vlFloatTCoords() {};
  int Allocate(const int sz, const int dim=2, const int ext=1000) {return this->TC.Allocate(dim*sz,dim*ext);};
  void Initialize() {return this->TC.Initialize();};
  char *GetClassName() {return "vlFloatTCoords";};

  // vlTCoords interface
  vlTCoords *MakeObject(int sze, int d=2, int ext=1000);
  int GetNumberOfTCoords() {return (this->TC.GetMaxId()+1)/this->Dimension;};
  void Squeeze() {this->TC.Squeeze();};
  float *GetTCoord(int i) {return this->TC.GetPtr(this->Dimension*i);};
  void SetTCoord(int i, float *tc);
  void InsertTCoord(int i, float *tc);
  int InsertNextTCoord(float *tc);

  // miscellaneous
  vlFloatTCoords &operator=(const vlFloatTCoords& ftc);
  void operator+=(const vlFloatTCoords& ftc) {this->TC += ftc.TC;};
  void Reset() {this->TC.Reset();};

protected:
  vlFloatArray TC;
};


inline void vlFloatTCoords::SetTCoord(int i, float *tc) 
{
  i*=this->Dimension; 
  for(int j=0;j<this->Dimension;j++) this->TC[i+j]=tc[j];
}

inline void vlFloatTCoords::InsertTCoord(int i, float *tc) 
{
  i*=this->Dimension; 
  for(int j=0; j<this->Dimension; j++) this->TC.InsertValue(i+j, tc[j]);
}

inline int vlFloatTCoords::InsertNextTCoord(float *tc) 
{
  int id = this->TC.InsertNextValue(tc[0]);
  for(int j=1; j<this->Dimension; j++) this->TC.InsertNextValue(tc[j]);
  return id/this->Dimension;
}

#endif
