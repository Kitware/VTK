/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTreeHeatmapItem - A 2D graphics item for rendering a tree.
//
// .SECTION Description
// This item draws a tree as a part of a vtkContextScene. This simple
// class has minimal state and delegates the determination of visual
// vertex and edge properties like color, size, width, etc. to
// a set of virtual functions. To influence the rendering of the tree,
// subclass this item and override the property functions you wish to
// customize.

#ifndef __vtkTreeHeatmapItem_h
#define __vtkTreeHeatmapItem_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkContextItem.h"
 
#include "vtkNew.h" // For vtkNew ivars

class vtkGraphLayout;
class vtkLookupTable;
class vtkTable;
class vtkTree;

class VTKVIEWSINFOVIS_EXPORT vtkTreeHeatmapItem : public vtkContextItem
{
public:
  static vtkTreeHeatmapItem *New();
  vtkTypeMacro(vtkTreeHeatmapItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // The tree that this item draws.
  virtual void SetTree(vtkTree *tree);
  vtkGetObjectMacro(Tree, vtkTree);
  
  // Description:
  // The table that this item draws.
  virtual void SetTable(vtkTable *table);
  vtkGetObjectMacro(Table, vtkTable);

protected:
  vtkTreeHeatmapItem();
  ~vtkTreeHeatmapItem();
    
  virtual void RebuildBuffers();
  virtual void PaintBuffers(vtkContext2D *painter);
  virtual bool IsDirty();

  void ComputeMultiplier();
  void InitializeLookupTable();

  // Description:
  // Paints the tree.
  virtual bool Paint(vtkContext2D *painter);

private:
  vtkTreeHeatmapItem(const vtkTreeHeatmapItem&); // Not implemented
  void operator=(const vtkTreeHeatmapItem&); // Not implemented

  vtkTree *Tree;
  vtkTree *LayoutTree;
  vtkTable *Table;
  unsigned long TreeHeatmapBuildTime;
  vtkNew<vtkGraphLayout> Layout;
  vtkNew<vtkLookupTable> LookupTable;
  double Multiplier;
};

#endif
