/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkCellTypes.h
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
#include "vtkCellType.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"


class VTK_EXPORT vtkCellTypes : public vtkObject 
{
public:
  static vtkCellTypes *New();
  vtkTypeMacro(vtkCellTypes,vtkObject);

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
  void DeleteCell(int cellId) { this->TypeArray->SetValue(cellId, VTK_EMPTY_CELL);};

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

protected:
  vtkCellTypes();
  ~vtkCellTypes();
  vtkCellTypes(const vtkCellTypes&);
  void operator=(const vtkCellTypes&);  

  vtkUnsignedCharArray *TypeArray; // pointer to types array
  vtkIntArray *LocationArray;   // pointer to array of offsets
  int Size;            // allocated size of data
  int MaxId;           // maximum index inserted thus far
  int Extend;          // grow array by this point
};



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


#endif
