/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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


=========================================================================*/
// .NAME vtkIdList - list of point or cell ids
// .SECTION Description
// vtkIdList is used to represent and pass data id's between
// objects. vtkIdList may represent any type of integer id, but
// usually represents point and cell ids.

#ifndef __vtkIdList_h
#define __vtkIdList_h

#include "vtkObject.h"

class VTK_EXPORT vtkIdList : public vtkObject
{
public:
  static vtkIdList *New();

  void Initialize();
  int Allocate(const int sz, const int strategy=0);
  vtkTypeMacro(vtkIdList,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the number of id's in the list.
  int GetNumberOfIds() {return this->NumberOfIds;};
  
  // Description:
  // Return the id at location i.
  int GetId(const int i) {return this->Ids[i];};
  
  // Description:
  // Specify the number of ids for this object to hold. Does an
  // allocation as well as setting the number of ids.
  void SetNumberOfIds(const int number);

  // Description:
  // Set the id at location i. Doesn't do range checking so it's a bit
  // faster than InsertId. Make sure you use SetNumberOfIds() to allocate
  // memory prior to using SetId().
  void SetId(const int i, const int id) {this->Ids[i] = id;};

  // Description:
  // Set the id at location i. Does range checking and allocates memory
  // as necessary.
  void InsertId(const int i, const int id);

  // Description:
  // Add the id specified to the end of the list. Range checking is performed.
  int InsertNextId(const int id);

  // Description:
  // If id is not already in list, insert it and return location in
  // list. Otherwise return just location in list.
  int InsertUniqueId(const int id);

  // Description:
  // Get a pointer to a particular data index.
  int *GetPointer(const int i) {return this->Ids + i;};

  // Description:
  // Get a pointer to a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  int *WritePointer(const int i, const int number);

  // Description:
  // Reset to an empty state.
  void Reset() {this->NumberOfIds = 0;};

  // Description:
  // Free any unused memory.
  void Squeeze() {this->Resize(this->NumberOfIds);};

  // Description:
  // Copy an id list by explicitly copying the internal array.
  void DeepCopy(vtkIdList *ids);

  // Description:
  // Delete specified id from list. Will remove all occurences of id in list.
  void DeleteId(int id);

  // Description:
  // Return -1 if id specified is not contained in the list; otherwise return
  // the position in the list.
  int IsId(int id);

  // Description:
  // Intersect this list with another vtkIdList. Updates current list according
  // to result of intersection operation.
  void IntersectWith(vtkIdList& otherIds);

protected:
  vtkIdList();
  ~vtkIdList();
  vtkIdList(const vtkIdList&) {};
  void operator=(const vtkIdList&) {};

  int NumberOfIds;
  int Size; 
  int *Ids;

  int *Resize(const int sz);
};

// In-lined for performance
inline int vtkIdList::InsertNextId(const int id)
{
  if ( this->NumberOfIds >= this->Size )
    {
    this->Resize(this->NumberOfIds+1);
    }
  this->Ids[this->NumberOfIds++] = id;
  return this->NumberOfIds-1;
}

inline int vtkIdList::IsId(int id)
{
  int *ptr, i;
  for (ptr=this->Ids, i=0; i<this->NumberOfIds; i++, ptr++)
    {
    if ( id == *ptr )
      {
      return i;
      }
    }
  return (-1);
}

#endif
