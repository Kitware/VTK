/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTypes.h
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
// .NAME vtkCellTypes - object provides direct access to cells in vtkCellArray and type information
// .SECTION Description
// This class is a supplemental object to vtkCellArray to allow random
// access into cells as well as representing cell type information.  The
// "location" field is the location in the vtkCellArray list in terms of an
// integer offset.  An integer offset was used instead of a pointer for easy
// storage and inter-process communication. The type information is defined
// in the file vtkCellType.h.
// .SECTION Caveats 
// Sometimes this class is used to pass type information independent of the
// random access (i.e., location) information. For example, see
// vtkDataSet::GetCellTypes(). If you use the class in this way, you can use
// a location value of -1.
// .SECTION See Also 
// vtkCellArray vtkCellLinks

#ifndef __vtkCellTypes_h
#define __vtkCellTypes_h

#include "vtkReferenceCount.h"
#include "vtkCellType.h"

struct _vtkCell_s {
    unsigned char type; //from CellType.h
    int loc; //location in associated CellArray object
};

class VTK_EXPORT vtkCellTypes : public vtkReferenceCount 
{
public:
  vtkCellTypes() : Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  vtkCellTypes(int sz, int ext);
  ~vtkCellTypes();
  static vtkCellTypes *New() {return new vtkCellTypes;};
  int Allocate(int sz=512, int ext=1000);
  const char *GetClassName() {return "vtkCellTypes";};

  void InsertCell(int id, unsigned char type, int loc);
  int InsertNextCell(unsigned char type, int loc);
  void DeleteCell(int cellId);

  //special operations to pass type information
  int GetNumberOfTypes();
  int IsType(unsigned char type);
  int InsertNextType(unsigned char type);
  unsigned char GetCellType(int id);

  //special operations used for managing data location offsets
  int GetCellLocation(int id);
  _vtkCell_s &GetCell(int id);

  void Squeeze();
  void Reset();

private:
  _vtkCell_s *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  _vtkCell_s *Resize(int sz);  // function to resize data
};

// Description:
// Return a reference to a cell list structure.
inline _vtkCell_s &vtkCellTypes::GetCell(int id) 
{
  return this->Array[id];
}

// Description:
// Return the type of cell.
inline unsigned char vtkCellTypes::GetCellType(int cellId) 
{
  return this->Array[cellId].type;
}

// Description:
// Return the location of the cell in the associated vtkCellArray.
inline int vtkCellTypes::GetCellLocation(int cellId) 
{
  return this->Array[cellId].loc;
}

// Description:
// Delete cell by setting to NULL cell type.
inline void vtkCellTypes::DeleteCell(int cellId)
{
  this->Array[cellId].type = VTK_NULL_ELEMENT;
}

// Description:
// Return the number of types in the list.
inline int vtkCellTypes::GetNumberOfTypes() 
{
  return (this->MaxId + 1);
}

// Description:
// Return 1 if type specified is contained in list; 0 otherwise.
inline int vtkCellTypes::IsType(unsigned char type)
{
  int numTypes=this->GetNumberOfTypes();

  for (int i=0; i<numTypes; i++) 
    if ( type == this->GetCellType(i)) return 1;
  return 0;
}

// Description:
// Add the type specified to the end of the list. Range checking is performed.
inline int vtkCellTypes::InsertNextType(unsigned char type) 
{
  return this->InsertNextCell(type,-1);
}

#endif
