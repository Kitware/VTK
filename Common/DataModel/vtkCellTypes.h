/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellTypes - object provides direct access to cells in vtkCellArray and type information
// .SECTION Description
// This class is a supplemental object to vtkCellArray to allow random access
// into cells as well as representing cell type information.  The "location"
// field is the location in the vtkCellArray list in terms of an integer
// offset.  An integer offset was used instead of a pointer for easy storage
// and inter-process communication. The type information is defined in the
// file vtkCellType.h.
//
// .SECTION Caveats
// Sometimes this class is used to pass type information independent of the
// random access (i.e., location) information. For example, see
// vtkDataSet::GetCellTypes(). If you use the class in this way, you can use
// a location value of -1.
//
// .SECTION See Also 
// vtkCellArray vtkCellLinks

#ifndef __vtkCellTypes_h
#define __vtkCellTypes_h

#include "vtkObject.h"

#include "vtkIntArray.h" // Needed for inline methods
#include "vtkUnsignedCharArray.h" // Needed for inline methods
#include "vtkCellType.h" // Needed for VTK_EMPTY_CELL

class VTK_FILTERING_EXPORT vtkCellTypes : public vtkObject 
{
public:
  static vtkCellTypes *New();
  vtkTypeMacro(vtkCellTypes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  int Allocate(int sz=512, int ext=1000);

  // Description:
  // Add a cell at specified id.
  void InsertCell(int id, unsigned char type, int loc);
  
  // Description:
  // Add a cell to the object in the next available slot.
  int InsertNextCell(unsigned char type, int loc);

  // Description:
  // Specify a group of cell types.
  void SetCellTypes(int ncells, vtkUnsignedCharArray *cellTypes, vtkIntArray *cellLocations);

  // Description:
  // Return the location of the cell in the associated vtkCellArray.
  int GetCellLocation(int cellId) { return this->LocationArray->GetValue(cellId);};

  // Description:
  // Delete cell by setting to NULL cell type.
  void DeleteCell(vtkIdType cellId) { this->TypeArray->SetValue(cellId, VTK_EMPTY_CELL);};

  // Description:
  // Return the number of types in the list.
  int GetNumberOfTypes() { return (this->MaxId + 1);};

  // Description:
  // Return 1 if type specified is contained in list; 0 otherwise.
  int IsType(unsigned char type);

  // Description:
  // Add the type specified to the end of the list. Range checking is performed.
  int InsertNextType(unsigned char type){return this->InsertNextCell(type,-1);};
  
  // Description:
  // Return the type of cell.
  unsigned char GetCellType(int cellId) { return this->TypeArray->GetValue(cellId);};

  // Description:
  // Reclaim any extra memory.
  void Squeeze();

  // Description:
  // Initialize object without releasing memory.
  void Reset();

  // Description:
  // Return the memory in kilobytes consumed by this cell type array. 
  // Used to support streaming and reading/writing data. The value 
  // returned is guaranteed to be greater than or equal to the memory 
  // required to actually represent the data represented by this object. 
  // The information returned is valid only after the pipeline has 
  // been updated.
  unsigned long GetActualMemorySize();

  // Description:
  // Standard DeepCopy method.  Since this object contains no reference
  // to other objects, there is no ShallowCopy.
  void DeepCopy(vtkCellTypes *src);

  // Description:
  // Given an int (as defined in vtkCellType.h) identifier for a class
  // return it's classname.
  static const char* GetClassNameFromTypeId(int typeId);

  // Description:
  // Given a data object classname, return it's int identified (as
  // defined in vtkCellType.h)
  static int GetTypeIdFromClassName(const char* classname);

  // Description:
  // This convenience method is a fast check to determine if a cell type
  // represents a linear or nonlinear cell.  This is generally much more
  // efficient than getting the appropriate vtkCell and checking its IsLinear
  // method.
  static int IsLinear(unsigned char type);

protected:
  vtkCellTypes();
  ~vtkCellTypes();

  vtkUnsignedCharArray *TypeArray; // pointer to types array
  vtkIntArray *LocationArray;   // pointer to array of offsets
  int Size;            // allocated size of data
  int MaxId;           // maximum index inserted thus far
  int Extend;          // grow array by this point
private:
  vtkCellTypes(const vtkCellTypes&);  // Not implemented.
  void operator=(const vtkCellTypes&);    // Not implemented.
};


//----------------------------------------------------------------------------
inline int vtkCellTypes::IsType(unsigned char type)
{
  int numTypes=this->GetNumberOfTypes();

  for (int i=0; i<numTypes; i++)
    {
    if ( type == this->GetCellType(i))
      {
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline int vtkCellTypes::IsLinear(unsigned char type)
{
  return (   (type <= 20)
          || (type == VTK_CONVEX_POINT_SET)
          || (type == VTK_POLYHEDRON) );
}


#endif
