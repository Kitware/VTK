/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IdList.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


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

class vtkIdList : public vtkObject
{
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
