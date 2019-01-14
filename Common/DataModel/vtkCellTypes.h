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
/**
 * @class   vtkCellTypes
 * @brief   object provides direct access to cells in vtkCellArray and type information
 *
 * This class is a supplemental object to vtkCellArray to allow random access
 * into cells as well as representing cell type information.  The "location"
 * field is the location in the vtkCellArray list in terms of an integer
 * offset.  An integer offset was used instead of a pointer for easy storage
 * and inter-process communication. The type information is defined in the
 * file vtkCellType.h.
 *
 * @warning
 * Sometimes this class is used to pass type information independent of the
 * random access (i.e., location) information. For example, see
 * vtkDataSet::GetCellTypes(). If you use the class in this way, you can use
 * a location value of -1.
 *
 * @sa
 * vtkCellArray vtkCellLinks
*/

#ifndef vtkCellTypes_h
#define vtkCellTypes_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkIntArray.h" // Needed for inline methods
#include "vtkIdTypeArray.h" // Needed for inline methods
#include "vtkUnsignedCharArray.h" // Needed for inline methods
#include "vtkCellType.h" // Needed for VTK_EMPTY_CELL

class VTKCOMMONDATAMODEL_EXPORT vtkCellTypes : public vtkObject
{
public:
  static vtkCellTypes *New();
  vtkTypeMacro(vtkCellTypes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Allocate memory for this array. Delete old storage only if necessary.
   */
  int Allocate(int sz=512, int ext=1000);

  /**
   * Add a cell at specified id.
   */
  void InsertCell(vtkIdType id, unsigned char type, vtkIdType loc);

  /**
   * Add a cell to the object in the next available slot.
   */
  vtkIdType InsertNextCell(unsigned char type, vtkIdType loc);

  /**
   * Specify a group of cell types.
   */
  void SetCellTypes(vtkIdType ncells, vtkUnsignedCharArray *cellTypes, vtkIdTypeArray *cellLocations);

  /**
   * Specify a group of cell types. This version is provided to maintain
   * backwards compatibility and does a copy of the cellLocations
   */
  void SetCellTypes(vtkIdType ncells, vtkUnsignedCharArray *cellTypes, vtkIntArray *cellLocations);

  /**
   * Return the location of the cell in the associated vtkCellArray.
   */
  vtkIdType GetCellLocation(vtkIdType cellId) { return this->LocationArray->GetValue(cellId);};

  /**
   * Delete cell by setting to nullptr cell type.
   */
  void DeleteCell(vtkIdType cellId) { this->TypeArray->SetValue(cellId, VTK_EMPTY_CELL);};

  /**
   * Return the number of types in the list.
   */
  vtkIdType GetNumberOfTypes() { return (this->MaxId + 1);};

  /**
   * Return 1 if type specified is contained in list; 0 otherwise.
   */
  int IsType(unsigned char type);

  /**
   * Add the type specified to the end of the list. Range checking is performed.
   */
  vtkIdType InsertNextType(unsigned char type){return this->InsertNextCell(type,-1);};

  /**
   * Return the type of cell.
   */
  unsigned char GetCellType(vtkIdType cellId) { return this->TypeArray->GetValue(cellId);};

  /**
   * Reclaim any extra memory.
   */
  void Squeeze();

  /**
   * Initialize object without releasing memory.
   */
  void Reset();

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this cell type array.
   * Used to support streaming and reading/writing data. The value
   * returned is guaranteed to be greater than or equal to the memory
   * required to actually represent the data represented by this object.
   * The information returned is valid only after the pipeline has
   * been updated.
   */
  unsigned long GetActualMemorySize();

  /**
   * Standard DeepCopy method.  Since this object contains no reference
   * to other objects, there is no ShallowCopy.
   */
  void DeepCopy(vtkCellTypes *src);

  /**
   * Given an int (as defined in vtkCellType.h) identifier for a class
   * return it's classname.
   */
  static const char* GetClassNameFromTypeId(int typeId);

  /**
   * Given a data object classname, return it's int identified (as
   * defined in vtkCellType.h)
   */
  static int GetTypeIdFromClassName(const char* classname);

  /**
   * This convenience method is a fast check to determine if a cell type
   * represents a linear or nonlinear cell.  This is generally much more
   * efficient than getting the appropriate vtkCell and checking its IsLinear
   * method.
   */
  static int IsLinear(unsigned char type);

protected:
  vtkCellTypes();
  ~vtkCellTypes() override;

  vtkUnsignedCharArray *TypeArray; // pointer to types array
  vtkIdTypeArray *LocationArray;   // pointer to array of offsets
  vtkIdType Size;            // allocated size of data
  vtkIdType MaxId;           // maximum index inserted thus far
  vtkIdType Extend;          // grow array by this point

private:
  vtkCellTypes(const vtkCellTypes&) = delete;
  void operator=(const vtkCellTypes&) = delete;
};


//----------------------------------------------------------------------------
inline int vtkCellTypes::IsType(unsigned char type)
{
  vtkIdType numTypes=this->GetNumberOfTypes();

  for (vtkIdType i=0; i<numTypes; i++)
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
