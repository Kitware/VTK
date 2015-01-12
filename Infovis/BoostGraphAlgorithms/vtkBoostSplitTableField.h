/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostSplitTableField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBoostSplitTableField - "Splits" one-or-more table fields by duplicating rows containing delimited data.
// .SECTION Description
//
// Assume the following table:
//
// Author                Year     Title
// Brian; Jeff; Tim      2007     Foo
// Tim                   2003     Bar
//
// If we produce a graph relating authors to the year in which they publish, the string
// "Brian; Jeff; Tim" will be treated (incorrectly) as a single author associated with
// the year 2007.  vtkBoostSplitTableField addresses this by splitting one-or-more fields into
// "subvalues" using a configurable delimiter and placing each subvalue on its own row
// (the other fields in the original row are copied).  Using the above example, splitting
// the "Author" field with a ";" (semicolon) delimiter produces:
//
// Author                Year     Title
// Brian                 2007     Foo
// Jeff                  2007     Foo
// Tim                   2007     Foo
// Tim                   2003     Bar
//
// When this table is converted to a graph, each author (correctly) becomes a separate node.
//
// Usage:
//
// Use AddField() to specify the field(s) to be split.  If no fields have been specified,
// vtkBoostSplitTableField will act as a passthrough.  By default, no fields are specified.
//
// The second argument to AddField() is a string containing zero-to-many single character
// delimiters (multi-character delimiters are not supported).
//
// If the input table is missing a field specified by AddField(), it is an error.
// If no fields are specified, no splitting is performed.
// If the delimiter for a field is an empty string, no splitting is performed on that field.

#ifndef vtkBoostSplitTableField_h
#define vtkBoostSplitTableField_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkStringArray;

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostSplitTableField : public vtkTableAlgorithm
{
public:
  static vtkBoostSplitTableField* New();
  vtkTypeMacro(vtkBoostSplitTableField, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void ClearFields();
  void AddField(const char* field, const char* delimiters);

protected:
  vtkBoostSplitTableField();
  ~vtkBoostSplitTableField();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  vtkStringArray* Fields;
  vtkStringArray* Delimiters;

private:
//BTX
  class implementation;
//ETX
  vtkBoostSplitTableField(const vtkBoostSplitTableField&); // Not implemented
  void operator=(const vtkBoostSplitTableField&);   // Not implemented
};

#endif
