/*=========================================================================

  Program:   Visualization Library
  Module:    FTensors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFloatTensors - floating point representation of tensor data
// .SECTION Description
// vlFloatTensors is a concrete implementation of vlTensors. Tensor values
// are represented using float values.

#ifndef __vlFloatTensors_h
#define __vlFloatTensors_h

#include "Tensors.hh"
#include "FArray.hh"

class vlFloatTensors : public vlTensors
{
public:
  vlFloatTensors() {};
  vlFloatTensors(const vlFloatTensors& ft);
  vlFloatTensors(int sz, int d=3, int ext=1000);
  ~vlFloatTensors() {};
  int Allocate(const int sz, const int dim=3, const int ext=1000);
  void Initialize() {this->T.Initialize();};
  char *GetClassName() {return "vlFloatTensors";};

  // vlTensors interface
  vlTensors *MakeObject(int sze, int d=3, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfTensors();
  void Squeeze() {this->T.Squeeze();};
  vlTensor &GetTensor(int i);
  void GetTensor(int i,vlTensor &t) {this->vlTensors::GetTensor(i,t);};
  void SetTensor(int i, vlTensor &t);
  void InsertTensor(int i, vlTensor &t);
  int InsertNextTensor(vlTensor &t);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vlFloatTensors &operator=(const vlFloatTensors& ft);
  void operator+=(const vlFloatTensors& ft) {this->T += ft.T;};
  void Reset() {this->T.Reset();};

protected:
  vlFloatArray T;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vlFloatTensors::GetPtr(const int id)
{
  return this->T.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of tensors to 
// write. Use the method WrotePtr() to mark completion of write.
// Make sure the dimension of the tensor is set prior to issuing this call.
inline float *vlFloatTensors::WritePtr(const int id, const int number)
{
  return this->T.WritePtr(id,this->Dimension*this->Dimension*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vlFloatTensors::WrotePtr() {}


inline vlFloatTensors::vlFloatTensors(const vlFloatTensors& ft) 
{
  this->T = ft.T;this->Dimension = ft.Dimension;
}

inline vlFloatTensors::vlFloatTensors(int sz, int d, int ext):
T(d*d*sz,d*d*ext) 
{
  this->Dimension=d;
}

inline int vlFloatTensors::Allocate(const int sz, const int dim,const int ext) 
{
  return this->T.Allocate(dim*dim*sz,dim*dim*ext);
}

inline int vlFloatTensors::GetNumberOfTensors() 
{
  return (this->T.GetMaxId()+1)/(this->Dimension*this->Dimension);
}

inline void vlFloatTensors::SetTensor(int id, vlTensor &t) 
{
  id *= this->Dimension*this->Dimension; 
  
  for (int j=0; j < this->Dimension; j++) 
    for (int i=0; i < this->Dimension; i++) 
      this->T[id+i+t.GetDimension()*j] = t.GetComponent(i,j);
}

inline void vlFloatTensors::InsertTensor(int id, vlTensor &t) 
{
  id *= this->Dimension*this->Dimension; 
  
  for (int j=0; j < this->Dimension; j++) 
    for (int i=0; i < this->Dimension; i++) 
      this->T.InsertValue(id+i+t.GetDimension()*j,t.GetComponent(i,j));
}

inline int vlFloatTensors::InsertNextTensor(vlTensor &t) 
{
  int id = this->GetNumberOfTensors() + 1;
  for (int j=0; j < this->Dimension; j++) 
    for (int i=0; i < this->Dimension; i++) 
      this->T.InsertNextValue(t.GetComponent(i,j));

  return id;
}

#endif
