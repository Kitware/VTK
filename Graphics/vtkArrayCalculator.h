/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayCalculator.h
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
// .NAME vtkArrayCalculator - perform mathematical operations on data in field data arrays
// .SECTION Description
// vtkArrayCalculator performs operations on vectors or scalars in field
// data arrays.  It uses vtkFunctionParser to do the parsing and to
// evaluate the function for each entry in the input arrays.  The arrays
// used in a given function must be all in point data or all in cell data.
// The resulting array will be stored as a field data array.  The result
// array can either be stored in a new array or it can overwrite an existing
// array.
// .SECTION See Also
// vtkFunctionParser

#ifndef __vtkArrayCalculator_h
#define __vtkArrayCalculator_h

#include "vtkDataSetToDataSetFilter.h"

class vtkFunctionParser;

#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

class VTK_GRAPHICS_EXPORT vtkArrayCalculator : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeRevisionMacro(vtkArrayCalculator,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkArrayCalculator *New();

  // Description:
  // Set/Get the function to be evaluated.
  void SetFunction(const char* function);
  vtkGetStringMacro(Function);
  
  // Description:
  // Add an array name to the list of arrays used in the function and specify
  // which components of the array to use in evaluating the function.  The
  // array name must match the name in the function.  Use AddScalarVariable or
  // AddVectorVariable to use a variable name different from the array name.
  void AddScalarArrayName(const char* arrayName, int component = 0);
  void AddVectorArrayName(const char* arrayName, int component0 = 0,
                          int component1 = 1, int component2 = 2);
  
  // Description:
  // Add a variable name, a corresponding array name, and which components of
  // the array to use.
  void AddScalarVariable(const char* variableName, const char* arrayName,
                         int component = 0);
  void AddVectorVariable(const char* variableName, const char* arrayName,
                         int component0 = 0, int component1 = 1,
                         int component2 = 2);
  
  // Description:
  // Set the name of the array in which to store the result of
  // evaluating this function.  If this is the name of an existing array,
  // that array will be overwritten.  Otherwise a new array will be
  // created with the specified name.
  void SetResultArrayName(const char* name);
  vtkGetStringMacro(ResultArrayName);
  
  // Description:
  // Control whether the filter operates on point data or cell data.
  // By default (AttributeModeToDefault), the filter uses point
  // data. Alternatively you can explicitly set the filter to use point data
  // (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  const char *GetAttributeModeAsString();

  // Description:
  // Remove all the variable names and their associated array names.
  void RemoveAllVariables();

  // Description:
  // Methods to get information about the current variables.
  char** GetScalarArrayNames() { return this->ScalarArrayNames; }
  char* GetScalarArrayName(int i);
  char** GetVectorArrayNames() { return this->VectorArrayNames; }
  char* GetVectorArrayName(int i);
  char** GetScalarVariableNames() { return this->ScalarVariableNames; }
  char* GetScalarVariableName(int i);
  char** GetVectorVariableNames() { return this->VectorVariableNames; }
  char* GetVectorVariableName(int i);
  int* GetSelectedScalarComponents() { return this->SelectedScalarComponents; }
  int GetSelectedScalarComponent(int i);
  int** GetSelectedVectorComponents() { return this->SelectedVectorComponents;}
  int* GetSelectedVectorComponents(int i);
  vtkGetMacro(NumberOfScalarArrays, int);
  vtkGetMacro(NumberOfVectorArrays, int);
  
protected:
  vtkArrayCalculator();
  ~vtkArrayCalculator();

  void Execute();
  
  char* Function;
  char* ResultArrayName;
  char** ScalarArrayNames;
  char** VectorArrayNames;
  char** ScalarVariableNames;
  char** VectorVariableNames;
  int NumberOfScalarArrays;
  int NumberOfVectorArrays;
  int AttributeMode;
  int* SelectedScalarComponents;
  int** SelectedVectorComponents;
  vtkFunctionParser* FunctionParser;
private:
  vtkArrayCalculator(const vtkArrayCalculator&);  // Not implemented.
  void operator=(const vtkArrayCalculator&);  // Not implemented.
};

#endif
