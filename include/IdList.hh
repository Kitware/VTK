/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IdList.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkIdList - list of point or cell ids
// .SECTION Description
// vtkIdLIst is used to represent and pass data id's between objects. vtkIdList
// may represent any type of integer id but usually represent point and
// cell ids.

#ifndef vtkIdList_h
#define vtkIdList_h

#include "Object.hh"
#include "IntArray.hh"

class vtkIdList {
public:
  vtkIdList(const int sz=128, const int ext=100):Ia(sz,ext) {};
  ~vtkIdList() {};
  vtkIdList &operator=(const vtkIdList& ids) {this->Ia = ids.Ia; return *this;};
  void Squeeze() {this->Ia.Squeeze();};

  int GetNumberOfIds() {return (this->Ia.GetMaxId() + 1);};
  int GetId(const int i) {return this->Ia[i];};
  void SetId(const int i, const int id) {this->Ia[i]=id;};
  void InsertId(const int i, const int id) {this->Ia.InsertValue(i,id);};
  int InsertNextId(const int id) {return this->Ia.InsertNextValue(id);};
  int getChunk(const int sz);
  void Reset() {this->Ia.Reset();};

  // special set operations
  void DeleteId(int Id);
  void IntersectWith(vtkIdList& otherIds);
  int IsId(int id);

protected:
  vtkIntArray Ia;
};

inline int vtkIdList::getChunk(const int sz) 
{ // get chunk of memory
  int pos = this->Ia.GetMaxId()+1;
  this->Ia.InsertValue(pos+sz-1,0);
  return pos;
}

inline int vtkIdList::IsId(int id)
{
  for(int i=0; i<this->GetNumberOfIds(); i++) 
    if(id == this->GetId(i)) return 1;
  return 0;
}


#endif
