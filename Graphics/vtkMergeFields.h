/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFields.h
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
// .NAME vtkMergeFields - Merge multiple fields into one.
// .SECTION Description
// vtkMergeFields is used to merge mutliple field into one.
// The new field is put in the same field data as the original field.
// For example
// @verbatim
// mf->SetOutputField("foo", vtkMergeFields::POINT_DATA);
// mf->SetNumberOfComponents(2);
// mf->Merge(0, "array1", 1);
// mf->Merge(1, "array2", 0);
// @endverbatim
// will tell vtkMergeFields to use the 2nd component of array1 and
// the 1st component of array2 to create a 2 component field called foo.
// The same can be done using Tcl:
// @verbatim
// mf SetOutputField foo POINT_DATA
// mf Merge 0 array1 1
// mf Merge 1 array2 0
//
// Field locations: DATA_OBJECT, POINT_DATA, CELL_DATA
// @endverbatim

// .SECTION See Also
// vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
// vtkDataSetAttributes vtkDataArray vtkRearrangeFields
// vtkSplitField vtkAssignAttribute

#ifndef __vtkMergeFields_h
#define __vtkMergeFields_h

#include "vtkDataSetToDataSetFilter.h"

class vtkFieldData;

class VTK_GRAPHICS_EXPORT vtkMergeFields : public vtkDataSetToDataSetFilter
{
public:
  vtkTypeMacro(vtkMergeFields,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new vtkMergeFields.
  static vtkMergeFields *New();

  // Description:
  // The output field will have the given name and it will be in
  // fieldLoc (the input fields also have to be in fieldLoc).
  void SetOutputField(const char* name, int fieldLoc);

  // Description:
  // Helper method used by the other language bindings. Allows the caller to
  // specify arguments as strings instead of enums.Returns an operation id 
  // which can later be used to remove the operation.
  void SetOutputField(const char* name, const char* fieldLoc);

  // Description:
  // Add a component (arrayName,sourceComp) to the output field.
  void Merge(int component, const char* arrayName, int sourceComp);

  // Description:
  // Set the number of the components in the output field.
  // This has to be set before execution. Default value is 0.
  vtkSetMacro(NumberOfComponents, int);

//BTX
  enum FieldLocations
  {
    DATA_OBJECT=0,
    POINT_DATA=1,
    CELL_DATA=2
  };
//ETX

//BTX
  struct Component
  {
    int Index;
    int SourceIndex;
    char* FieldName;   
    Component* Next;   // linked list
    void SetName(const char* name)
      {
	delete[] this->FieldName;
	this->FieldName = 0;
	if (name)
	  {
	  this->FieldName = new char[strlen(name)+1];
	  strcpy(this->FieldName, name);
	  }
      }
    Component() { FieldName = 0; }
    ~Component() { delete[] FieldName; }
  };
//ETX

protected:

//BTX
  enum FieldType
  {
    NAME,
    ATTRIBUTE
  };
//ETX

  vtkMergeFields();
  virtual ~vtkMergeFields();

  void Execute();

  char* FieldName;
  int FieldLocation;
  int NumberOfComponents;
  int OutputDataType;

  static char FieldLocationNames[3][12];


  int MergeArray(vtkDataArray* in, vtkDataArray* out, int inComp, int outComp);

  // Components are stored as a linked list.
  Component* Head;
  Component* Tail;

  // Methods to browse/modify the linked list.
  Component* GetNextComponent(Component* op)
    { return op->Next; }
  Component* GetFirst()
    { return this->Head; }
  void AddComponent(Component* op);
  Component* FindComponent(int index);
  void DeleteAllComponents();

  void PrintComponent(Component* op, ostream& os, vtkIndent indent)
    {
      os << indent << "Field name: " << op->FieldName << endl;
      os << indent << "Component index: " << op->Index << endl;
      os << indent << "Source component index: " << op->SourceIndex << endl;
    }

  void PrintAllComponents(ostream& os, vtkIndent indent)
    {
      Component* cur = this->GetFirst();
      if (!cur) { return; }
      Component* before;
      do
	{
	before = cur;
	cur = cur->Next;
	os << endl;
	this->PrintComponent(before, os, indent);
	} 
      while (cur);
    }
private:
  vtkMergeFields(const vtkMergeFields&);  // Not implemented.
  void operator=(const vtkMergeFields&);  // Not implemented.
};

#endif


