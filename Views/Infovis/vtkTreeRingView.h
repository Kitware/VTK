/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingView.h

  -------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTreeRingView - Displays a tree in concentric rings.
//
// .SECTION Description
// Accepts a graph and a hierarchy - currently
// a tree - and provides a hierarchy-aware display.  Currently, this means
// displaying the hierarchy using a tree ring layout, then rendering the graph
// vertices as leaves of the tree with curved graph edges between leaves.
//
// .SEE ALSO
// vtkGraphLayoutView
//
// .SECTION Thanks
// Thanks to Jason Shepherd for implementing this class

#ifndef vtkTreeRingView_h
#define vtkTreeRingView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkTreeAreaView.h"

class VTKVIEWSINFOVIS_EXPORT vtkTreeRingView : public vtkTreeAreaView
{
public:
  static vtkTreeRingView *New();
  vtkTypeMacro(vtkTreeRingView, vtkTreeAreaView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the root angles for laying out the hierarchy.
  void SetRootAngles(double start, double end);

  // Description:
  // Sets whether the root is at the center or around the outside.
  virtual void SetRootAtCenter(bool value);
  virtual bool GetRootAtCenter();
  vtkBooleanMacro(RootAtCenter, bool);

  // Description:
  // Set the thickness of each layer.
  virtual void SetLayerThickness(double thickness);
  virtual double GetLayerThickness();

  // Description:
  // Set the interior radius of the tree
  // (i.e. the size of the "hole" in the center).
  virtual void SetInteriorRadius(double thickness);
  virtual double GetInteriorRadius();

  // Description:
  // Set the log spacing factor for the invisible interior tree
  // used for routing edges of the overlaid graph.
  virtual void SetInteriorLogSpacingValue(double thickness);
  virtual double GetInteriorLogSpacingValue();

protected:
  vtkTreeRingView();
  ~vtkTreeRingView();

private:
  vtkTreeRingView(const vtkTreeRingView&);  // Not implemented.
  void operator=(const vtkTreeRingView&);  // Not implemented.
};

#endif
