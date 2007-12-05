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

// vtkSelection is a tree data structure used to store selection parameters. 
// Each node in this tree stores a list of properties (in a vtkInformation) 
// and a list of selection values (in a vtkAbstractArray). The properties 
// provide information about what the selection values mean. For example the 
// CONTENT_TYPE property gives information about what is stored by the node. 
// If the CONTENT_TYPE is SELECTIONS, the vtkSelection is used as a parent 
// node that contains other vtkSelections. If the CONTENT_TYPE is GLOBALIDS,
// the SelectionList array should contain a list of cell or point ids, which
// identify the particular cells or points that have matching values in the 
// GLOBALID vtkDataSetAttribute array. If the CONTENT_TYPE is PEDIGREEIDS, the
// SelectionList array should contain a list of cell or point ids, which identify
// the particular cells or points that have matching valuse in the PEDIGREEID
// vtkDataSetAttribute array. The FIELD_TYPE property designates whether the 
// selection refers to cells or points.
//
// Usually, each node under the root is a selection from
// one data object. SOURCE or SOURCE_ID properties point to this object. If
// the selection was performed on a renderer, PROP or PROP_ID point to the
// prop the selection was made on. Selection nodes corresponding to
// composite datasets may contain child nodes. Each child node of a
// composite dataset should have GROUP and BLOCK set. This way, the pointer
// to the composite dataset can be obtained from the parent of a block
// node. The pointer to the block can be obtained from the composite
// dataset using GROUP and BLOCK.  
//
// .SECTION Caveats 
// Each node can have one parent and should not be added to more than one
// node as child. No SelectionList is created by default. It should be assigned.

#ifndef __vtkSelection_h
#define __vtkSelection_h

#include "vtkDataObject.h"

//BTX
class vtkAbstractArray;
class vtkFieldData;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkTable;
struct vtkSelectionInternals;
//ETX

class VTK_FILTERING_EXPORT vtkSelection : public vtkDataObject
{
public:
  vtkTypeRevisionMacro(vtkSelection,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSelection* New();

  // Description:
  // Restore data object to initial state,
  virtual void Initialize();
  
  // Description:
  // Returns VTK_SELECTION enumeration value.
  virtual int GetDataObjectType() {return VTK_SELECTION;}

  // Description:
  // Sets the selection list.
  virtual void SetSelectionList(vtkAbstractArray*);

  // Description:
  // Returns the selection list.
  virtual vtkAbstractArray* GetSelectionList();

  // Description:
  // Sets the selection table.
  virtual void SetSelectionData(vtkFieldData* data) { this->SetFieldData(data); }
  
  // Description:
  // Returns the selection table.
  virtual vtkFieldData* GetSelectionData() { return this->GetFieldData(); }
  
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
  virtual void RemoveAllChildren();

  // Description: 
  // Removes all properties and children. Removes selection list array.
  // Does not change parent node.
  virtual void Clear();

  // Description: 
  // Copy properties, selection list and children of the input.
  virtual void DeepCopy(vtkDataObject* src);

  // Description: 
  // Copy properties, selection list and children of the input.
  // This is a shallow copy: selection lists and pointers in the
  // properties are passed by reference.
  virtual void ShallowCopy(vtkDataObject* src);

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
  // Union this selection with the specified selection.
  // The selection types must be identical.
  virtual void Union(vtkSelection* selection);

  // Description:
  // Return the MTime taking into account changes to the properties
  unsigned long GetMTime();

  // vtkSelection specific keys follow:
  // Description:
  // Get the (primary) property that describes the content of a selection 
  // node's data. Other auxiliary description properties follow.
  // SELECTIONS means that a vtkSelection contains sub selections.
  // GLOBALIDS means that the selection list contains values from the
  // vtkDataSetAttribute array of the same name.
  // PEDIGREEIDS means that the selection list contains values from the
  // vtkDataSetAttribute array of the same name.
  // VALUES means the the selection list contains values from an 
  // arbitrary attribute array (ignores any globalids attribute)
  // INDICES means that the selection list contains indexes into the 
  // cell or point arrays.
  // FRUSTUM means the set of points and cells inside a frustum
  // LOCATIONS means the set of points and cells near a set of positions
  // THRESHOLDS means the points and cells with values within a set of ranges
  // GetContentType() returns -1 if the content type is not set.
  static vtkInformationIntegerKey* CONTENT_TYPE();
//BTX
  enum SelectionContent
  {
    SELECTIONS,
    COMPOSITE_SELECTIONS,
    GLOBALIDS,
    PEDIGREEIDS,
    VALUES,
    INDICES,
    FRUSTUM,
    LOCATIONS,
    THRESHOLDS
  };
//ETX
  
  // Description:
  // Get or set the content type of the selection.
  // This is the same as setting the CONTENT_TYPE() key on the property.
  virtual void SetContentType(int type);
  virtual int GetContentType();

  // Description:
  // Controls whether cell, point, or field data determine what is inside and out.
  // The default is CELL.
  // GetFieldType() returns -1 if the field type is not set.
  static vtkInformationIntegerKey* FIELD_TYPE();
//BTX
  enum SelectionField
  {
    CELL,
    POINT,
    FIELD
  };
//ETX
  
  // Description:
  // Get or set the field type of the selection.
  // This is the same as setting the FIELD_TYPE() key on the property.
  virtual void SetFieldType(int type);
  virtual int GetFieldType();
  
  // Description:
  // For location selection of points, if distance is greater than this reject.
  static vtkInformationDoubleKey* EPSILON();

  // Description:
  // This flag tells the extraction filter not to convert the selected
  // output into an unstructured grid, but instead to produce a vtkInsidedness
  // array and add it to the input dataset. 
  static vtkInformationIntegerKey* PRESERVE_TOPOLOGY();

  // Description:
  // This flag tells the extraction filter, when FIELD_TYPE==POINT, that
  // it should also extract the cells that contain any of the extracted points.
  static vtkInformationIntegerKey* CONTAINING_CELLS();

  // Description:
  // This flag tells the extraction filter to exclude the selection.
  static vtkInformationIntegerKey* INVERSE();

  // Description:
  // A helper for visible cell selector, this is the number of pixels covered
  // by the actor whose cells are listed in the selection.
  static vtkInformationIntegerKey* PIXEL_COUNT();

  // Description:
  // Pointer to the data or algorithm the selection belongs to.
  static vtkInformationObjectBaseKey* SOURCE();

  // Description:
  // ID of the data or algorithm the selection belongs to. What
  // ID means is application specific.
  static vtkInformationIntegerKey* SOURCE_ID();

  // Description:
  // Pointer to the prop the selection belongs to.
  static vtkInformationObjectBaseKey* PROP();

  // Description:
  // ID of the prop the selection belongs to. What
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

  // Description:
  // This key is used when making visible vertex selection. It means
  // that the cell ID selection has data about which vertices for each
  // cell are visible.
  static vtkInformationIntegerKey* INDEXED_VERTICES();

  // Description:
  // Retrieve a vtkSelection stored inside an invormation object.
  static vtkSelection* GetData(vtkInformation* info);
  static vtkSelection* GetData(vtkInformationVector* v, int i=0);

  // Description:
  // For Frustum selection.
  static vtkInformationIntegerKey* SHOW_BOUNDS();

//BTX
protected:
  vtkSelection();
  ~vtkSelection();

  // Description:
  // Merges the selection list between self and the other. Assumes that both has
  // identical properties.
  void UnionSelectionList(vtkSelection* other);


  // Description:
  // Compares Properties of self and other to ensure that they are exactly same.
  bool EqualProperties(vtkSelection* other, bool fullcompare=true);

  vtkInformation* Properties;
  vtkSelection* ParentNode;

private:
  vtkSelection(const vtkSelection&);  // Not implemented.
  void operator=(const vtkSelection&);  // Not implemented.

  vtkSelectionInternals* Internal;
//ETX
};

#endif
