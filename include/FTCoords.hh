/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FTCoords.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFloatTCoords - floating point representation of texture coordinates
// .SECTION Description
// vtkFloatTCoords is a concrete implementation of vtkTCoords. Texture
// coordinates are represented using float values.

#ifndef __vtkFloatTCoords_h
#define __vtkFloatTCoords_h

#include "TCoords.hh"
#include "FArray.hh"

class vtkFloatTCoords : public vtkTCoords
{
public:
  vtkFloatTCoords() {};
  vtkFloatTCoords(const vtkFloatTCoords& ftc) {this->TC = ftc.TC;this->Dimension = ftc.Dimension;};
  vtkFloatTCoords(int sz, int d=2, int ext=1000):TC(d*sz,d*ext) {this->Dimension=d;};
  ~vtkFloatTCoords() {};
  int Allocate(const int sz, const int dim=2, const int ext=1000) {return this->TC.Allocate(dim*sz,dim*ext);};
  void Initialize() {this->TC.Initialize();};
  char *GetClassName() {return "vtkFloatTCoords";};

  // vtkTCoords interface
  vtkTCoords *MakeObject(int sze, int d=2, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfTCoords() {return (this->TC.GetMaxId()+1)/this->Dimension;};
  void Squeeze() {this->TC.Squeeze();};
  float *GetTCoord(int i) {return this->TC.GetPtr(this->Dimension*i);};
  void GetTCoord(int i,float tc[3]) {this->vtkTCoords::GetTCoord(i,tc);};
  void SetTCoord(int i, float *tc);
  void InsertTCoord(int i, float *tc);
  int InsertNextTCoord(float *tc);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatTCoords &operator=(const vtkFloatTCoords& ftc);
  void operator+=(const vtkFloatTCoords& ftc) {this->TC += ftc.TC;};
  void Reset() {this->TC.Reset();};

protected:
  vtkFloatArray TC;
};


// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatTCoords::GetPtr(const int id)
{
  return this->TC.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of texture coordinates to 
// write. Use the method WrotePtr() to mark completion of write.
// Make sure the dimension of the texture coordinate is set prior to issuing 
// this call.
inline float *vtkFloatTCoords::WritePtr(const int id, const int number)
{
  return this->TC.WritePtr(id,this->Dimension*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatTCoords::WrotePtr() {}


inline void vtkFloatTCoords::SetTCoord(int i, float *tc) 
{
  i*=this->Dimension; 
  for(int j=0;j<this->Dimension;j++) this->TC[i+j]=tc[j];
}

inline void vtkFloatTCoords::InsertTCoord(int i, float *tc) 
{
  i*=this->Dimension; 
  for(int j=0; j<this->Dimension; j++) this->TC.InsertValue(i+j, tc[j]);
}

inline int vtkFloatTCoords::InsertNextTCoord(float *tc) 
{
  int id = this->TC.InsertNextValue(tc[0]);
  for(int j=1; j<this->Dimension; j++) this->TC.InsertNextValue(tc[j]);
  return id/this->Dimension;
}

#endif
