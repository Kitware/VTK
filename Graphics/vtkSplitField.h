/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitField.h
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
// .NAME vtkSplitField - Split a field into single component fields
// .SECTION Description
// vtkSplitField is used to split a multi-component field (vtkDataArray)
// into multiple single component fields. The new fields are put in
// the same field data as the original field. The output arrays
// are of the same type as the input array. Example:
// @verbatim
// sf->SetInputField("gradient", vtkSplitField::POINT_DATA);
// sf->Split(0, "firstcomponent");
// @endverbatim
// tells vtkSplitField to extract the first component of the field
// called gradient and create an array called firstcomponent (the
// new field will be in the output's point data).
// The same can be done from Tcl:
// @verbatim
// sf SetInputField gradient POINT_DATA
// sf Split 0 firstcomponent
//
// AttributeTypes: SCALARS, VECTORS, NORMALS, TCOORDS, TENSORS
// Field locations: DATA_OBJECT, POINT_DATA, CELL_DATA
// @endverbatim
// Note that, by default, the original array is also passed through.

// .SECTION Caveats
// When using Tcl, Java, Python or Visual Basic bindings, the array name 
// can not be one of the  AttributeTypes when calling Split() which takes
// strings as arguments. The Tcl (Java etc.) command will
// always assume the string corresponds to an attribute type when
// the argument is one of the AttributeTypes. In this situation,
// use the Split() which takes enums.

// .SECTION See Also
// vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
// vtkDataSetAttributes vtkDataArray vtkRearrangeFields
// vtkAssignAttribute vtkMergeFields

#ifndef __vtkSplitField_h
#define __vtkSplitField_h

#include "vtkDataSetToDataSetFilter.h"

class vtkFieldData;

class VTK_EXPORT vtkSplitField : public vtkDataSetToDataSetFilter
{
public:
  vtkTypeMacro(vtkSplitField,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new vtkSplitField.
  static vtkSplitField *New();

  // Description:
  // Use the  given attribute in the field data given
  // by fieldLoc as input.
  void SetInputField(int attributeType, int fieldLoc);

  // Description:
  // Use the array with given name in the field data given
  // by fieldLoc as input.
  void SetInputField(const char* name, int fieldLoc);

  // Description:
  // Helper method used by other language bindings. Allows the caller to
  // specify arguments as strings instead of enums.
  void SetInputField(const char* name, const char* fieldLoc);

  // Description:
  // Create a new array with the given component.
  void Split(int component, const char* arrayName);

//BTX
  enum FieldLocation
  {
    DATA_OBJECT=0,
    POINT_DATA=1,
    CELL_DATA=2
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

  vtkSplitField();
  virtual ~vtkSplitField();
  vtkSplitField(const vtkSplitField&);
  void operator=(const vtkSplitField&);

  void Execute();

  char* FieldName;
  int FieldType;
  int AttributeType;
  int FieldLocation;

  static char FieldLocationNames[3][12];
  static char AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10];

  vtkDataArray* SplitArray(vtkDataArray* da, int component);

//BTX
  struct Component
  {
    int Index;
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
};

#endif


