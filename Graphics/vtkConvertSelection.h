/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkConvertSelection - 
//
// .SECTION Description
// vtkConvertSelection
//
// .SECTION Thanks
//
// .SECTION See Also

#ifndef __vtkConvertSelection_h
#define __vtkConvertSelection_h

#include "vtkSelectionAlgorithm.h"

class vtkIdTypeArray;
class vtkSelection;
class vtkStringArray;

class VTK_GRAPHICS_EXPORT vtkConvertSelection : public vtkSelectionAlgorithm 
{
public:
  static vtkConvertSelection *New();
  vtkTypeRevisionMacro(vtkConvertSelection, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The output selection type.
  // This should be one of the constants defined in vtkSelection.h.
  vtkSetMacro(OutputType, int);
  vtkGetMacro(OutputType, int);
  
  // Description:
  // The output array name for value or threshold selections.
  virtual void SetArrayName(const char*);
  virtual const char* GetArrayName();
  
  // Description:
  // The output array names for value selection.
  virtual void SetArrayNames(vtkStringArray*);
  vtkGetObjectMacro(ArrayNames, vtkStringArray);
  
  // Description:
  // Static methods for easily converting between selection types.
  // NOTE: The returned selection pointer IS reference counted,
  // so be sure to Delete() it when you are done with it.
  static vtkSelection* ToIndexSelection(
    vtkSelection* input, 
    vtkDataObject* data);
  static vtkSelection* ToGlobalIdSelection(
    vtkSelection* input, 
    vtkDataObject* data);
  static vtkSelection* ToPedigreeIdSelection(
    vtkSelection* input, 
    vtkDataObject* data);
  static vtkSelection* ToValueSelection(
    vtkSelection* input, 
    vtkDataObject* data, 
    const char* arrayName);
  static vtkSelection* ToValueSelection(
    vtkSelection* input, 
    vtkDataObject* data, 
    vtkStringArray* arrayNames);
  
  // Description:
  // A generic static method for converting selection types.
  // The type should be an integer constant defined in vtkSelection.h.
  static vtkSelection* ToSelectionType(
    vtkSelection* input, 
    vtkDataObject* data, 
    int type, 
    vtkStringArray* arrayNames = 0);
protected:
  vtkConvertSelection();
  ~vtkConvertSelection();

  virtual int RequestData(
    vtkInformation *, 
    vtkInformationVector **, 
    vtkInformationVector *);
  
  int Convert(
    vtkSelection* input,
    vtkDataObject* data,
    vtkSelection* output);

  int ConvertToIndexSelection(
    vtkSelection* input, 
    vtkDataSet* data,
    vtkSelection* output);
  
  int SelectTableFromTable(
    vtkTable* selTable,
    vtkTable* dataTable,
    vtkIdTypeArray* indices);

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);
  
  int OutputType;
  vtkStringArray* ArrayNames;

private:
  vtkConvertSelection(const vtkConvertSelection&);  // Not implemented.
  void operator=(const vtkConvertSelection&);  // Not implemented.
};

#endif
