/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkList.h
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
// .NAME vtkLinkList - object represents upward pointers from points to list of cells using each point
// .SECTION Description
// vtkLinkList is a supplemental object to vtkCellArray and vtkCellList, 
// enabling access from points to the cells using the points. vtkLinkList is
// a list of Links, each link represents a dynamic list of cell id's using the 
// point. The information provided by this object can be used to determine 
// neighbors and construct other local topological information.
// .SECTION See Also
// vtkCellArray vtkCellList

#ifndef __vtkLinkList_h
#define __vtkLinkList_h

#include "vtkRefCount.h"
class vtkDataSet;

struct _vtkLink_s {
    unsigned short ncells;
    int *cells;
};

class VTK_EXPORT vtkLinkList : public vtkRefCount 
{
public:
  vtkLinkList():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  vtkLinkList(int sz, int ext=1000);
  ~vtkLinkList();
  char *GetClassName() {return "vtkLinkList";};

  _vtkLink_s &GetLink(int ptId);
  unsigned short GetNcells(int ptId);
  void BuildLinks(vtkDataSet *data);
  int *GetCells(int ptId);
  int InsertNextPoint(int numLinks);
  void InsertNextCellReference(int ptId, int cellId);

  void DeletePoint(int ptId);
  void RemoveCellReference(int cellId, int ptId);
  void AddCellReference(int cellId, int ptId);
  void ResizeCellList(int ptId, int size);

  void Squeeze();
  void Reset();

private:
  void IncrementLinkCount(int ptId);
  void AllocateLinks(int n);
  void InsertCellReference(int ptId, unsigned short pos, int cellId);

  _vtkLink_s *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  _vtkLink_s *Resize(int sz);  // function to resize data
};

// Description:
// Get a link structure given a point id.
inline _vtkLink_s &vtkLinkList::GetLink(int ptId) {return this->Array[ptId];};

// Description:
// Get the number of cells using the point specified by ptId.
inline unsigned short vtkLinkList::GetNcells(int ptId) 
{
  return this->Array[ptId].ncells;
}

// Description:
// Return a list of cell ids using the point.
inline int *vtkLinkList::GetCells(int ptId) {return this->Array[ptId].cells;};

// Description:
// Increment the count of the number of cells using the point.
inline void vtkLinkList::IncrementLinkCount(int ptId) 
{
  this->Array[ptId].ncells++;
}

// Description:
// Insert a cell id into the list of cells using the point.
inline void vtkLinkList::InsertCellReference(int ptId, unsigned short pos, int cellId) 
{
  this->Array[ptId].cells[pos] = cellId;
}

// Description:
// Delete point (and storage) by destroying links to using cells.
inline void vtkLinkList::DeletePoint(int ptId)
{
  this->Array[ptId].ncells = 0;
  delete [] this->Array[ptId].cells;
  this->Array[ptId].cells = NULL;
}

// Description:
// Insert a cell id into the list of cells (at the end) using the cell id 
// provided. (Make sure to extend the link list (if necessary) using the
// method ResizeCellList().)
inline void vtkLinkList::InsertNextCellReference(int ptId, int cellId) 
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

// Description:
// Delete the reference to the cell (cellId) from the point (ptId). This
// removes the reference to the cellId from the cell list, but does not resize
// the list (recover memory with ResizeCellList(), if necessary).
inline void vtkLinkList::RemoveCellReference(int cellId, int ptId)
{
  int *cells=this->Array[ptId].cells;
  int ncells=this->Array[ptId].ncells;

  for (int i=0; i < ncells; i++)
    {
    if (cells[i] == cellId)
      {
      for (int j=i; j < (ncells-1); j++) cells[j] = cells[j+1];
      this->Array[ptId].ncells--;
      break;
      }
    }
}

// Description:
// Add the reference to the cell (cellId) from the point (ptId). This
// adds a reference to the cellId from the cell list, but does not resize
// the list (extend memory with ResizeCellList(), if necessary).
inline void vtkLinkList::AddCellReference(int cellId, int ptId)
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

// Description:
// Change the length of a point's link list (i.e., list of cells using a point)
// by the size specified. 
inline void vtkLinkList::ResizeCellList(int ptId, int size)
{
  int *cells, newSize;

  newSize = this->Array[ptId].ncells + size;
  cells = new int[newSize];
  memcpy(cells, this->Array[ptId].cells, this->Array[ptId].ncells*sizeof(int));
  delete [] this->Array[ptId].cells;
  this->Array[ptId].cells = cells;
}

#endif

