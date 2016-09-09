/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelection - A node in a selection tree. Used to store selection results.
// .SECTION Description

// vtkSelection is a collection of vtkSelectionNode objects, each of which
// contains information about a piece of the whole selection. Each selection
// node may contain different types of selections.
//
// .SECTION See Also
// vtkSelectionNode

#ifndef vtkSelection_h
#define vtkSelection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkSelectionNode;
struct vtkSelectionInternals;

class VTKCOMMONDATAMODEL_EXPORT vtkSelection : public vtkDataObject
{
public:
  vtkTypeMacro(vtkSelection,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkSelection* New();

  // Description:
  // Restore data object to initial state,
  void Initialize() VTK_OVERRIDE;

  // Description:
  // Returns VTK_SELECTION enumeration value.
  int GetDataObjectType() VTK_OVERRIDE  {return VTK_SELECTION;}

  // Description:
  // Returns the number of nodes in this selection.
  // Each node contains information about part of the selection.
  unsigned int GetNumberOfNodes();

  // Description:
  // Returns a node given it's index. Performs bound checking
  // and will return 0 if out-of-bounds.
  virtual vtkSelectionNode* GetNode(unsigned int idx);

  // Description:
  // Adds a selection node.
  virtual void AddNode(vtkSelectionNode*);

  // Description:
  // Removes a selection node.
  virtual void RemoveNode(unsigned int idx);
  virtual void RemoveNode(vtkSelectionNode*);
  virtual void RemoveAllNodes();

  // Description:
  // Copy selection nodes of the input.
  void DeepCopy(vtkDataObject* src) VTK_OVERRIDE;

  // Description:
  // Copy selection nodes of the input.
  // This is a shallow copy: selection lists and pointers in the
  // properties are passed by reference.
  void ShallowCopy(vtkDataObject* src) VTK_OVERRIDE;

  // Description:
  // Union this selection with the specified selection.
  // Attempts to reuse selection nodes in this selection if properties
  // match exactly. Otherwise, creates new selection nodes.
  virtual void Union(vtkSelection* selection);

  // Description:
  // Union this selection with the specified selection node.
  // Attempts to reuse a selection node in this selection if properties
  // match exactly. Otherwise, creates a new selection node.
  virtual void Union(vtkSelectionNode* node);

  // Description:
  // Remove the nodes from the specified selection from this selection.
  // Assumes that selection node internal arrays are vtkIdTypeArrays.
  virtual void Subtract(vtkSelection* selection);

  // Description:
  // Remove the nodes from the specified selection from this selection.
  // Assumes that selection node internal arrays are vtkIdTypeArrays.
  virtual void Subtract(vtkSelectionNode* node);

  // Description:
  // Return the MTime taking into account changes to the properties
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  // Description:
  // Dumps the contents of the selection, giving basic information only.
  virtual void Dump();

  virtual void Dump(ostream& os);

  // Description:
  // Retrieve a vtkSelection stored inside an invormation object.
  static vtkSelection* GetData(vtkInformation* info);
  static vtkSelection* GetData(vtkInformationVector* v, int i=0);

protected:
  vtkSelection();
  ~vtkSelection();

private:
  vtkSelection(const vtkSelection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSelection&) VTK_DELETE_FUNCTION;

  vtkSelectionInternals* Internal;

};

#endif
