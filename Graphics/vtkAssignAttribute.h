/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignAttribute.h
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
// .NAME vtkAssignAttribute - Labels a field as an attribute
// .SECTION Description
// vtkAssignAttribute is use to label a field (vtkDataArray) as an attribute.
// A field name or an attribute to labeled can be specified. For example:
// @verbatim
// aa->Assign("foo", vtkDataSetAttributes::SCALARS, 
//            vtkAttributeLocation::POINT_DATA);
// @endverbatim
// tells vtkAssignAttribute to make the array in the point data called
// "foo" the active scalars. On the other hand,
// @verbatim
// aa->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, 
//            vtkAttributeLocation::POINT_DATA);
// @endverbatim
// tells vtkAssignAttribute to make the active vectors also the active
// scalars. The same can be done more easily from Tcl by using the Assign()
// method which takes strings:
// @verbatim
// aa Assign "foo" SCALARS POINT_DATA 
// or
// aa Assign SCALARS VECTORS POINT_DATA
//
// AttributeTypes: SCALARS, VECTORS, NORMALS, TCOORDS, TENSORS
// Attribute locations: POINT_DATA, CELL_DATA
// @endverbatim

// .SECTION Caveats
// When using Tcl, Java, Python or Visual Basic bindings, the array name 
// can not be one of the  AttributeTypes when calling Assign() which takes
// strings as arguments. The Tcl (Java etc.) command will
// always assume the string corresponds to an attribute type when
// the argument is one of the AttributeTypes. In this situation,
// use the Assign() which takes enums.

// .SECTION See Also
// vtkFieldData vtkDataSet vtkDataObjectToDataSetFilter
// vtkDataSetAttributes vtkDataArray vtkRearrangeFields
// vtkSplitField vtkMergeFields

#ifndef __vtkAssignAttribute_h
#define __vtkAssignAttribute_h

#include "vtkDataSetToDataSetFilter.h"

class vtkFieldData;

class VTK_GRAPHICS_EXPORT vtkAssignAttribute : public vtkDataSetToDataSetFilter
{
public:
  vtkTypeMacro(vtkAssignAttribute,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new vtkAssignAttribute.
  static vtkAssignAttribute *New();

  // Description:
  // Label an attribute as another attribute.
  void Assign(int inputAttributeType, int attributeType, int attributeLoc);

  // Description:
  // Label an array as an attribute.
  void Assign(const char* fieldName, int attributeType, int attributeLoc);

  // Description:
  // Helper method used by other language bindings. Allows the caller to
  // specify arguments as strings instead of enums.
  void Assign(const char* name, const char* attributeType, 
	      const char* attributeLoc);


//BTX
  enum AttributeLocation
  {
    POINT_DATA=0,
    CELL_DATA=1
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

  vtkAssignAttribute();
  virtual ~vtkAssignAttribute();
  vtkAssignAttribute(const vtkAssignAttribute&);
  void operator=(const vtkAssignAttribute&);

  void Execute();

  char* FieldName;
  int FieldType;
  int AttributeType;
  int InputAttributeType;
  int AttributeLocation;

  static char AttributeLocationNames[2][12];
  static char AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10];
};

#endif


