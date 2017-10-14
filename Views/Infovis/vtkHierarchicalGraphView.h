/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalGraphView.h

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

/**
 * @class   vtkHierarchicalGraphView
 * @brief   Accepts a graph and a hierarchy - currently
 * a tree - and provides a hierarchy-aware display.  Currently, this means
 * displaying the hierarchy using a tree layout, then rendering the graph
 * vertices as leaves of the tree with curved graph edges between leaves.
 *
 *
 * Takes a graph and a hierarchy (currently a tree) and lays out the graph
 * vertices based on their categorization within the hierarchy.
 *
 * .SEE ALSO
 * vtkGraphLayoutView
 *
 * @par Thanks:
 * Thanks to the turtle with jets for feet, without you this class wouldn't
 * have been possible.
*/

#ifndef vtkHierarchicalGraphView_h
#define vtkHierarchicalGraphView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkGraphLayoutView.h"

class vtkRenderedHierarchyRepresentation;

class VTKVIEWSINFOVIS_EXPORT vtkHierarchicalGraphView : public vtkGraphLayoutView
{
public:
  static vtkHierarchicalGraphView *New();
  vtkTypeMacro(vtkHierarchicalGraphView, vtkGraphLayoutView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the tree and graph representations to the appropriate input ports.
   */
  vtkDataRepresentation* SetHierarchyFromInputConnection(vtkAlgorithmOutput* conn);
  vtkDataRepresentation* SetHierarchyFromInput(vtkDataObject* input);
  vtkDataRepresentation* SetGraphFromInputConnection(vtkAlgorithmOutput* conn);
  vtkDataRepresentation* SetGraphFromInput(vtkDataObject* input);
  //@}

  //@{
  /**
   * The array to use for edge labeling.  Default is "label".
   */
  virtual void SetGraphEdgeLabelArrayName(const char* name);
  virtual const char* GetGraphEdgeLabelArrayName();
  //@}

  //@{
  /**
   * Whether to show edge labels.  Default is off.
   */
  virtual void SetGraphEdgeLabelVisibility(bool vis);
  virtual bool GetGraphEdgeLabelVisibility();
  vtkBooleanMacro(GraphEdgeLabelVisibility, bool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  virtual void SetGraphEdgeColorArrayName(const char* name);
  virtual const char* GetGraphEdgeColorArrayName();
  //@}

  /**
   * Set the color to be the spline fraction
   */
  virtual void SetGraphEdgeColorToSplineFraction();

  //@{
  /**
   * Whether to color edges.  Default is off.
   */
  virtual void SetColorGraphEdgesByArray(bool vis);
  virtual bool GetColorGraphEdgesByArray();
  vtkBooleanMacro(ColorGraphEdgesByArray, bool);
  //@}

  //@{
  /**
   * Set the bundling strength.
   */
  virtual void SetBundlingStrength(double strength);
  virtual double GetBundlingStrength();
  //@}

  //@{
  /**
   * Whether the graph edges are visible (default off).
   */
  virtual void SetGraphVisibility(bool b);
  virtual bool GetGraphVisibility();
  vtkBooleanMacro(GraphVisibility, bool);
  //@}

  //@{
  /**
   * The size of the font used for edge labeling
   */
  virtual void SetGraphEdgeLabelFontSize(const int size);
  virtual int GetGraphEdgeLabelFontSize();
  //@}

protected:
  vtkHierarchicalGraphView();
  ~vtkHierarchicalGraphView() override;

  //@{
  /**
   * Overrides behavior in vtkGraphLayoutView to create a
   * vtkRenderedHierarchyRepresentation by default.
   */
  vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn) override;
  vtkRenderedGraphRepresentation* GetGraphRepresentation() override;
  virtual vtkRenderedHierarchyRepresentation* GetHierarchyRepresentation();
  //@}

private:
  vtkHierarchicalGraphView(const vtkHierarchicalGraphView&) = delete;
  void operator=(const vtkHierarchicalGraphView&) = delete;
};

#endif
