/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellList.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkCellList - object provides direct access to cells in vtkCellArray
// .SECTION Description
// Supplemental object to vtkCellArray to allow random access into cells.
// The "location" field is the location in the vtkCellArray list in terms of 
// an integer offset.  An integer offset was used instead of a pointer for 
// easy storage and inter-process communication.
// .SECTION See Also
// vtkCellArray vtkLinkList

#ifndef __vtkCellList_h
#define __vtkCellList_h

#include "vtkRefCount.h"
#include "vtkCellType.h"

struct _vtkCell_s {
    unsigned char type; //from CellTypes.h
    int loc; //location in associated CellArray object
};

class VTK_EXPORT vtkCellList : public vtkRefCount 
{
public:
  vtkCellList() : Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  vtkCellList(const int sz, const int ext);
  ~vtkCellList();
  vtkCellList *New() {return new vtkCellList;};
  char *GetClassName() {return "vtkCellList";};

  _vtkCell_s &GetCell(const int id);
  unsigned char GetCellType(const int id);
  int GetCellLocation(const int id);
  void InsertCell(const int id, const unsigned char type, const int loc);
  int InsertNextCell(const unsigned char type, const int loc);

  void DeleteCell(int cellId);

  void Squeeze();
  void Reset();

private:
  _vtkCell_s *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  _vtkCell_s *Resize(const int sz);  // function to resize data
};

// Description:
// Return a reference to a cell list structure.
inline _vtkCell_s &vtkCellList::GetCell(const int id) 
{
  return this->Array[id];
}

// Description:
// Return the type of cell.
inline unsigned char vtkCellList::GetCellType(const int cellId) 
{
  return this->Array[cellId].type;
}

// Description:
// Return the location of the cell in the associated vtkCellArray.
inline int vtkCellList::GetCellLocation(const int cellId) 
{
  return this->Array[cellId].loc;
}

// Description:
// Delete cell by setting to NULL cell type.
inline void vtkCellList::DeleteCell(int cellId)
{
  this->Array[cellId].type = VTK_NULL_ELEMENT;
}

#endif
