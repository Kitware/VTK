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
// .NAME vtkSelection - A node in a selection tree. Used to store selection results
// .SECTION Description

// vtkSelection is a node of a tree data structure used to store
// selection results. Each node in this tree stores a list of properties
// (as a vtkInformation map) and a list of selection values (as a
// vtkAbstractArray). The properties provide information about what the
// selection values mean. For example, if the CONTENT_TYPE properties gives
// information about what is stored by the node. If the CONTENT_TYPE is
// SELECTIONS, the node is used as a parent node that contains other
// vtkSelections and does not usually contain any selection values. If
// the CONTENT_TYPE is CELL_IDS, the SelectionList array should contain a
// list of cells ids. Usually, each node under the root is a selection from
// one data object. SOURCE or SOURCE_ID properties point to this object. If
// the selection was performed on a renderer, PROP or PROP_ID point to the
// prop the selection was made on. Selection nodes corresponding to
// composite datasets may contain child nodes. Each childe node of a
// composite dataset should have GROUP and BLOCK set. This was, the pointer
// to the composite dataset can be obtained from the parent of a block
// node. The pointer to the block can be obtained from the composite
// dataset using GROUP and BLOCK.  
//
// .SECTION Caveats 
// Each node can have one parent and should not be added to more than one
// node as child. No SelectionList is created by default. It should be assigned.

#ifndef __vtkSelection_h
#define __vtkSelection_h

#include "vtkObject.h"

//BTX
class vtkAbstractArray;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
struct vtkSelectionInternals;
//ETX

class VTK_FILTERING_EXPORT vtkSelection : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSelection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSelection* New();

  // Description:
  // Sets the selection list.
  virtual void SetSelectionList(vtkAbstractArray*);

  // Description:
  // Returns the selection list.
  vtkGetObjectMacro(SelectionList, vtkAbstractArray);

  // Description:
  // Returns the property map.
  vtkGetObjectMacro(Properties, vtkInformation);

  // Description: 
  // Returns the number of children.
  unsigned int GetNumberOfChildren();

  // Description: 
  // Returns a child given it's index. Performs bound checking
  // and will return 0 if out-of-bounds.
  virtual vtkSelection* GetChild(unsigned int idx);

  // Description: 
  // Returns the parent of the selection node unless it is root.
  // A child does not keep a reference to the parent to avoid
  // reference loops.
  virtual vtkSelection* GetParentNode()
    {
      return this->ParentNode;
    }

  // Description: 
  // Adds a child node. If the node is already a child, it is not
  // added a second time. Note that a node can be a child of only
  // one node at a time. This method will also set the parent of
  // the passed node to this.
  virtual void AddChild(vtkSelection*);

  // Description: 
  // Removes a child. It will also set the parent of the removed
  // child to be 0.
  virtual void RemoveChild(unsigned int idx);
  virtual void RemoveChild(vtkSelection*);

  // Description: 
  // Removes all properties and children. Removes selection list array.
  // Does not change parent node.
  virtual void Clear();

  // Description: 
  // Copy properties, selection list and children of the input.
  virtual void DeepCopy(vtkSelection* input);

  // Description: 
  // Add the children of the given selection to this one. 
  // This requires that both selection objects have a SELECTIONS
  // CONTENT_TYPE. Note that this does not check if a child with
  // exact same properties exists before adding. If any child node
  // that contains other selections is found in the input, it's
  // children are added to a selection node of the same SOURCE_ID
  // or PROP_ID. This handles the case of assemblies and composite
  // datasets.
  virtual void CopyChildren(vtkSelection*);

  // Description:
  // Return the MTime taking into account changes to the properties
  unsigned long GetMTime();

  // vtkSelection specific keys.

  // Description:
  // The content of the selection node. See SelectionContent
  // enum for the possible values.
  static vtkInformationIntegerKey* CONTENT_TYPE();

  // Description:
  // Pointer to the data or algorithm the selection belongs to.
  static vtkInformationObjectBaseKey* SOURCE();

  // Description:
  // ID the data or algorithm the selection belongs to. What
  // ID means is application specific.
  static vtkInformationIntegerKey* SOURCE_ID();

  // Description:
  // Pointer to the prop the selection belongs to.
  static vtkInformationObjectBaseKey* PROP();

  // Description:
  // ID the prop the selection belongs to. What
  // ID means is application specific.
  static vtkInformationIntegerKey* PROP_ID();

  // Description:
  // Process id the selection is on.
  static vtkInformationIntegerKey* PROCESS_ID();

  // Description:
  // The composite data group the selection belongs to.
  static vtkInformationIntegerKey* GROUP();

  // Description:
  // The composite data block the selection belongs to.
  static vtkInformationIntegerKey* BLOCK();

//BTX
  enum SelectionContent
  {
    SELECTIONS,
    COMPOSITE_SELECTIONS,
    POINT_IDS,
    POINT_ID_RANGE,
    GLOBAL_POINT_IDS,
    GLOBAL_POINT_ID_RANGE,
    CELL_IDS,
    CELL_ID_RANGE,
    GLOBAL_CELL_IDS,
    GLOBAL_CELL_ID_RANGE
  };
//ETX

protected:
  vtkSelection();
  ~vtkSelection();

  vtkInformation* Properties;
  vtkAbstractArray* SelectionList;
  vtkSelection* ParentNode;

private:
  vtkSelection(const vtkSelection&);  // Not implemented.
  void operator=(const vtkSelection&);  // Not implemented.

  vtkSelectionInternals* Internal;
};

#endif
