/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLinks.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCellLinks - object represents upward pointers from points to list of cells using each point
// .SECTION Description
// vtkCellLinks is a supplemental object to vtkCellArray and vtkCellTypes, 
// enabling access from points to the cells using the points. vtkCellLinks is
// a list of Links, each link represents a dynamic list of cell id's using the 
// point. The information provided by this object can be used to determine 
// neighbors and construct other local topological information.
// .SECTION See Also
// vtkCellArray vtkCellTypes

#ifndef __vtkCellLinks_h
#define __vtkCellLinks_h

#include "vtkObject.h"
class vtkDataSet;
class vtkCellArray;

struct _vtkLink_s {
    unsigned short ncells;
    vtkIdType *cells;
};

class VTK_COMMON_EXPORT vtkCellLinks : public vtkObject 
{
public:
  static vtkCellLinks *New();
  vtkTypeMacro(vtkCellLinks,vtkObject);

  // Description:
  // Allocate the specified number of links (i.e., number of points) that
  // will be built.
  void Allocate(vtkIdType numLinks, vtkIdType ext=1000);

  // Description:
  // Get a link structure given a point id.
  _vtkLink_s &GetLink(vtkIdType ptId) {return this->Array[ptId];};

  // Description:
  // Get the number of cells using the point specified by ptId.
  unsigned short GetNcells(vtkIdType ptId) { return this->Array[ptId].ncells;};

  // Description:
  // Build the link list array.
  void BuildLinks(vtkDataSet *data);

  // Description:
  // Build the link list array.
  void BuildLinks(vtkDataSet *data, vtkCellArray *Connectivity);

  // Description:
  // Return a list of cell ids using the point.
  vtkIdType *GetCells(vtkIdType ptId) {return this->Array[ptId].cells;};

  // Description:
  // Insert a new point into the cell-links data structure. The size parameter
  // is the initial size of the list.
  vtkIdType InsertNextPoint(int numLinks);

  // Description:
  // Insert a cell id into the list of cells (at the end) using the cell id 
  // provided. (Make sure to extend the link list (if necessary) using the
  // method ResizeCellList().)
  void InsertNextCellReference(vtkIdType ptId, vtkIdType cellId);

  // Description:
  // Delete point (and storage) by destroying links to using cells.
  void DeletePoint(vtkIdType ptId);

  // Description:
  // Delete the reference to the cell (cellId) from the point (ptId). This
  // removes the reference to the cellId from the cell list, but does not
  // resize the list (recover memory with ResizeCellList(), if necessary).
  void RemoveCellReference(vtkIdType cellId, vtkIdType ptId);

  // Description:
  // Add the reference to the cell (cellId) from the point (ptId). This
  // adds a reference to the cellId from the cell list, but does not resize
  // the list (extend memory with ResizeCellList(), if necessary).
  void AddCellReference(vtkIdType cellId, vtkIdType ptId);

  // Description:
  // Change the length of a point's link list (i.e., list of cells using a
  // point) by the size specified.
  void ResizeCellList(vtkIdType ptId, int size);

  // Description:
  // Reclaim any unused memory.
  void Squeeze();

  // Description:
  // Reset to a state of no entries without freeing the memory.
  void Reset();

  // Description:
  // Return the memory in kilobytes consumed by this cell links array. 
  // Used to support streaming and reading/writing data. The value 
  // returned is guaranteed to be greater than or equal to the memory 
  // required to actually represent the data represented by this object. 
  // The information returned is valid only after the pipeline has 
  // been updated.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Standard DeepCopy method.  Since this object contains no reference
  // to other objects, there is no ShallowCopy.
  void DeepCopy(vtkCellLinks *src);

protected:
  vtkCellLinks():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  ~vtkCellLinks();
  vtkCellLinks(const vtkCellLinks&);
  void operator=(const vtkCellLinks&);

  // Description:
  // Increment the count of the number of cells using the point.
  void IncrementLinkCount(vtkIdType ptId) { this->Array[ptId].ncells++;};

  void AllocateLinks(vtkIdType n);

  // Description:
  // Insert a cell id into the list of cells using the point.
  void InsertCellReference(vtkIdType ptId, unsigned short pos,
                           vtkIdType cellId);

  _vtkLink_s *Array;   // pointer to data
  vtkIdType Size;       // allocated size of data
  vtkIdType MaxId;     // maximum index inserted thus far
  vtkIdType Extend;     // grow array by this point
  _vtkLink_s *Resize(vtkIdType sz);  // function to resize data
};


inline void vtkCellLinks::InsertCellReference(vtkIdType ptId,
                                              unsigned short pos,
                                              vtkIdType cellId) 
{
  this->Array[ptId].cells[pos] = cellId;
}

inline void vtkCellLinks::DeletePoint(vtkIdType ptId)
{
  this->Array[ptId].ncells = 0;
  delete [] this->Array[ptId].cells;
  this->Array[ptId].cells = NULL;
}

inline void vtkCellLinks::InsertNextCellReference(vtkIdType ptId,
                                                  vtkIdType cellId) 
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

inline void vtkCellLinks::RemoveCellReference(vtkIdType cellId, vtkIdType ptId)
{
  vtkIdType *cells=this->Array[ptId].cells;
  int ncells=this->Array[ptId].ncells;

  for (int i=0; i < ncells; i++)
    {
    if (cells[i] == cellId)
      {
      for (int j=i; j < (ncells-1); j++)
	{
	cells[j] = cells[j+1];
	}
      this->Array[ptId].ncells--;
      break;
      }
    }
}

inline void vtkCellLinks::AddCellReference(vtkIdType cellId, vtkIdType ptId)
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

inline void vtkCellLinks::ResizeCellList(vtkIdType ptId, int size)
{
  int newSize;
  vtkIdType *cells;
  
  newSize = this->Array[ptId].ncells + size;
  cells = new vtkIdType[newSize];
  memcpy(cells, this->Array[ptId].cells,
         this->Array[ptId].ncells*sizeof(vtkIdType));
  delete [] this->Array[ptId].cells;
  this->Array[ptId].cells = cells;
}

#endif

