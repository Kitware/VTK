/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignAttribute.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
  vtkTypeRevisionMacro(vtkAssignAttribute,vtkDataSetToDataSetFilter);
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

  void Execute();

  char* FieldName;
  int FieldType;
  int AttributeType;
  int InputAttributeType;
  int AttributeLocation;

  static char AttributeLocationNames[2][12];
  static char AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10];
private:
  vtkAssignAttribute(const vtkAssignAttribute&);  // Not implemented.
  void operator=(const vtkAssignAttribute&);  // Not implemented.
};

#endif


