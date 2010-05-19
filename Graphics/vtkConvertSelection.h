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
// .NAME vtkConvertSelection - Convert a selection from one type to another
//
// .SECTION Description
// vtkConvertSelection converts an input selection from one type to another
// in the context of a data object being selected. The first input is the
// selection, while the second input is the data object that the selection
// relates to.
//
// .SECTION See Also
// vtkSelection vtkSelectionNode vtkExtractSelection vtkExtractSelectedGraph

#ifndef __vtkConvertSelection_h
#define __vtkConvertSelection_h

#include "vtkSelectionAlgorithm.h"

class vtkCompositeDataSet;
class vtkGraph;
class vtkIdTypeArray;
class vtkSelection;
class vtkSelectionNode;
class vtkStringArray;
class vtkTable;

class VTK_GRAPHICS_EXPORT vtkConvertSelection : public vtkSelectionAlgorithm 
{
public:
  static vtkConvertSelection *New();
  vtkTypeMacro(vtkConvertSelection, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A convenience method for setting the second input (i.e. the data object).
  void SetDataObjectConnection(vtkAlgorithmOutput* in);

  // Description:
  // The input field type.
  // If this is set to a number other than -1, ignores the input selection
  // field type and instead assumes that all selection nodes have the
  // field type specified.
  // This should be one of the constants defined in vtkSelectionNode.h.
  // Default is -1.
  vtkSetMacro(InputFieldType, int);
  vtkGetMacro(InputFieldType, int);
  
  // Description:
  // The output selection content type.
  // This should be one of the constants defined in vtkSelectionNode.h.
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
  // Convenience methods used by UI
  void AddArrayName(const char*);
  void ClearArrayNames();
  
  // Description:
  // When on, creates a separate selection node for each array.
  // Defaults to OFF.
  vtkSetMacro(MatchAnyValues, bool);
  vtkGetMacro(MatchAnyValues, bool);
  vtkBooleanMacro(MatchAnyValues, bool);

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
  // Static generic method for obtaining selected items from a data object.
  // Other static methods (e.g. GetSelectedVertices) call this one.
  static void GetSelectedItems(
    vtkSelection* input,
    vtkDataObject* data,
    int fieldType,
    vtkIdTypeArray* indices);

  // Description:
  // Static methods for easily obtaining selected items from a data object.
  // The array argument will be filled with the selected items.
  static void GetSelectedVertices(
    vtkSelection* input,
    vtkGraph* data,
    vtkIdTypeArray* indices);
  static void GetSelectedEdges(
    vtkSelection* input,
    vtkGraph* data,
    vtkIdTypeArray* indices);
  static void GetSelectedPoints(
    vtkSelection* input,
    vtkDataSet* data,
    vtkIdTypeArray* indices);
  static void GetSelectedCells(
    vtkSelection* input,
    vtkDataSet* data,
    vtkIdTypeArray* indices);
  static void GetSelectedRows(
    vtkSelection* input,
    vtkTable* data,
    vtkIdTypeArray* indices);
  
  // Description:
  // A generic static method for converting selection types.
  // The type should be an integer constant defined in vtkSelectionNode.h.
  static vtkSelection* ToSelectionType(
    vtkSelection* input, 
    vtkDataObject* data, 
    int type, 
    vtkStringArray* arrayNames = 0,
    int inputFieldType = -1);
    
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

  int ConvertCompositeDataSet(
    vtkSelection* input,
    vtkCompositeDataSet* data,
    vtkSelection* output);

  int ConvertToIndexSelection(
    vtkSelectionNode* input, 
    vtkDataSet* data,
    vtkSelectionNode* output);
  
  int SelectTableFromTable(
    vtkTable* selTable,
    vtkTable* dataTable,
    vtkIdTypeArray* indices);

  int ConvertToBlockSelection(
    vtkSelection* input, vtkCompositeDataSet* data, vtkSelection* output);

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);
  
  int OutputType;
  int InputFieldType;
  vtkStringArray* ArrayNames;
  bool MatchAnyValues;

private:
  vtkConvertSelection(const vtkConvertSelection&);  // Not implemented.
  void operator=(const vtkConvertSelection&);  // Not implemented.
};

#endif
