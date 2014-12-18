/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAddMembershipArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkAddMembershipArray - Add an array to the output indicating
// membership within an input selection.
//
// .SECTION Description
// This filter takes an input selection, vtkDataSetAttribute
// information, and data object and adds a bit array to the output
// vtkDataSetAttributes indicating whether each index was selected or not.

#ifndef vtkAddMembershipArray_h
#define vtkAddMembershipArray_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkAbstractArray;

class VTKINFOVISCORE_EXPORT vtkAddMembershipArray : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAddMembershipArray* New();
  vtkTypeMacro(vtkAddMembershipArray,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5
    };
  //ETX

  // Description:
  // The field type to add the membership array to.
  vtkGetMacro(FieldType, int);
  vtkSetClampMacro(FieldType, int, 0, 5);

  // Description:
  // The name of the array added to the output vtkDataSetAttributes
  // indicating membership. Defaults to "membership".
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);

  vtkSetStringMacro(InputArrayName);
  vtkGetStringMacro(InputArrayName);

  void SetInputValues(vtkAbstractArray*);
  vtkGetObjectMacro(InputValues,vtkAbstractArray);

protected:
  vtkAddMembershipArray();
  ~vtkAddMembershipArray();

  int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  int FieldType;
  char* OutputArrayName;
  char* InputArrayName;
//BTX
  vtkAbstractArray* InputValues;
//ETX

private:
  vtkAddMembershipArray(const vtkAddMembershipArray&); // Not implemented
  void operator=(const vtkAddMembershipArray&);   // Not implemented
};

#endif

