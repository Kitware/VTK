/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendSelection.h

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
/**
 * @class   vtkAppendSelection
 * @brief   appends one or more selections together
 *
 *
 * vtkAppendSelection is a filter that appends one of more selections into
 * a single selection.  All selections must have the same content type unless
 * AppendByUnion is false.
*/

#ifndef vtkAppendSelection_h
#define vtkAppendSelection_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

class vtkSelection;

class VTKFILTERSCORE_EXPORT vtkAppendSelection : public vtkSelectionAlgorithm
{
public:
  static vtkAppendSelection *New();

  vtkTypeMacro(vtkAppendSelection,vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * UserManagedInputs allows the user to set inputs by number instead of
   * using the AddInput/RemoveInput functions. Calls to
   * SetNumberOfInputs/SetInputByNumber should not be mixed with calls
   * to AddInput/RemoveInput. By default, UserManagedInputs is false.
   */
  vtkSetMacro(UserManagedInputs,int);
  vtkGetMacro(UserManagedInputs,int);
  vtkBooleanMacro(UserManagedInputs,int);
  //@}

  /**
   * Add a dataset to the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber instead.
   */
  void AddInputData(vtkSelection *);

  /**
   * Remove a dataset from the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber (NULL) instead.
   */
  void RemoveInputData(vtkSelection *);

  //@{
  /**
   * Get any input of this filter.
   */
  vtkSelection *GetInput(int idx);
  vtkSelection *GetInput() { return this->GetInput( 0 ); };
  //@}

  /**
   * Directly set(allocate) number of inputs, should only be used
   * when UserManagedInputs is true.
   */
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputConnectionByNumber(int num, vtkAlgorithmOutput *input);

  //@{
  /**
   * When set to true, all the selections are combined together to form a single
   * vtkSelection output.
   * When set to false, the output is a composite selection with
   * input selections as the children of the composite selection. This allows
   * for selections with different content types and properties. Default is
   * true.
   */
  vtkSetMacro(AppendByUnion, int);
  vtkGetMacro(AppendByUnion, int);
  vtkBooleanMacro(AppendByUnion, int);
  //@}

protected:
  vtkAppendSelection();
  ~vtkAppendSelection() VTK_OVERRIDE;

  // Usual data generation method
  int RequestData(vtkInformation *,
                  vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int, vtkInformation *) VTK_OVERRIDE;

 private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInputData(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkSelection not a vtkDataObject."); };

  int UserManagedInputs;
  int AppendByUnion;
private:
  vtkAppendSelection(const vtkAppendSelection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAppendSelection&) VTK_DELETE_FUNCTION;
};

#endif


