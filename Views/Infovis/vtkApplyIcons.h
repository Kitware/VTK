/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApplyIcons.h

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
// .NAME vtkApplyIcons - apply icons to a data set.
//
// .SECTION Description
// vtkApplyIcons performs a iconing of the dataset using default icons,
// lookup tables, annotations, and/or a selection. The output is a
// vtkIntArray containing the icon index for each
// element in the dataset. The first input is the dataset to be iconed, which
// may be a vtkTable, vtkGraph subclass, or vtkDataSet subclass.
//
// The second (optional) input is a vtkAnnotationLayers object, which stores
// a list of annotation layers, with each layer holding a list of
// vtkAnnotation objects. The annotation specifies a subset of data along with
// other properties, including icon. For annotations with icon properties,
// this algorithm will use the icon index of annotated elements,
// using a "top one wins" strategy.
//
// The third (optional) input is a vtkSelection object, meant for specifying
// the current selection. You can control the icon of the selection, or whether
// there is a set of selected icons at a particular offset in the icon sheet.
//
// The algorithm takes an input array, specified with
// SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name)
// This sets data arrays to use to icon the data with
// the associated lookup table. For vtkGraph and vtkTable inputs, you would use
// FIELD_ASSOCIATION_VERTICES, FIELD_ASSOCIATION_EDGES, or
// FIELD_ASSOCIATION_ROWS as appropriate. The icon array will be added to the same
// set of attributes that the input array came from. If there is no input array,
// the icon array will be applied to the attributes associated with the
// AttributeType parameter.
//
// Icons are assigned with the following priorities:
// <ol>
// <li> If an item is part of the selection, it is glyphed with that icon.
// <li> Otherwise, if the item is part of an annotation, it is glyphed
//      with the icon of the final (top) annotation in the set of layers.
// <li> Otherwise, if a lookup table is used, it is glyphed using the
//      lookup table icon for the data value of the element.
// <li> Otherwise it will be glyphed with the default icon.
// </ol>

#ifndef __vtkApplyIcons_h
#define __vtkApplyIcons_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkVariant.h" // For variant arguments.

class VTKVIEWSINFOVIS_EXPORT vtkApplyIcons : public vtkPassInputTypeAlgorithm
{
public:
  static vtkApplyIcons *New();
  vtkTypeMacro(vtkApplyIcons, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Edits the lookup table to use for point icons. This is only used if
  // input array 0 is set and UsePointLookupTable is on.
  void SetIconType(vtkVariant v, int icon);
  void SetIconType(double v, int icon)
    { this->SetIconType(vtkVariant(v), icon); }
  void SetIconType(const char* v, int icon)
    { this->SetIconType(vtkVariant(v), icon); }
  void ClearAllIconTypes();

  // Description:
  // If on, uses the point lookup table to set the colors of unannotated,
  // unselected elements of the data.
  vtkSetMacro(UseLookupTable, bool);
  vtkGetMacro(UseLookupTable, bool);
  vtkBooleanMacro(UseLookupTable, bool);

  // Description:
  // The default point icon for all unannotated, unselected elements
  // of the data. This is used if UsePointLookupTable is off.
  vtkSetMacro(DefaultIcon, int);
  vtkGetMacro(DefaultIcon, int);

  // Description:
  // The point icon for all selected elements of the data.
  // This is used if the annotation input has a current selection.
  vtkSetMacro(SelectedIcon, int);
  vtkGetMacro(SelectedIcon, int);

  // Description:
  // The output array name for the point icon index array.
  // Default is "vtkApplyIcons icon".
  vtkSetStringMacro(IconOutputArrayName);
  vtkGetStringMacro(IconOutputArrayName);

  //BTX
  enum
  {
    SELECTED_ICON,
    SELECTED_OFFSET,
    ANNOTATION_ICON,
    IGNORE_SELECTION
  };
  //ETX

  // Description:
  // Changes the behavior of the icon to use for selected items.
  // <ul>
  // <li>SELECTED_ICON uses SelectedIcon as the icon for all selected elements.
  // <li>SELECTED_OFFSET uses SelectedIcon as an offset to add to all selected elements.
  // <li>ANNOTATION_ICON uses the ICON_INDEX() property of the current annotation.
  // <li>IGNORE_SELECTION does not change the icon based on the current selection.
  // </ul>
  // The default is IGNORE_SELECTION.
  vtkSetMacro(SelectionMode, int);
  vtkGetMacro(SelectionMode, int);
  virtual void SetSelectionModeToSelectedIcon()
    { this->SetSelectionMode(SELECTED_ICON); }
  virtual void SetSelectionModeToSelectedOffset()
    { this->SetSelectionMode(SELECTED_OFFSET); }
  virtual void SetSelectionModeToAnnotationIcon()
    { this->SetSelectionMode(ANNOTATION_ICON); }
  virtual void SetSelectionModeToIgnoreSelection()
    { this->SetSelectionMode(IGNORE_SELECTION); }

  // Description:
  // The attribute type to append the icon array to, used only if the
  // input array is not specified or does not exist. This is set to one
  // of the AttributeTypes enum in vtkDataObject (e.g. POINT, CELL, VERTEX
  // EDGE, FIELD).
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);

protected:
  vtkApplyIcons();
  ~vtkApplyIcons();

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

  int DefaultIcon;
  int SelectedIcon;
  bool UseLookupTable;
  char* IconOutputArrayName;
  int SelectionMode;
  int AttributeType;

  //BTX
  class Internals;
  Internals* Implementation;
  //ETX

private:
  vtkApplyIcons(const vtkApplyIcons&);  // Not implemented.
  void operator=(const vtkApplyIcons&);  // Not implemented.
};

#endif
