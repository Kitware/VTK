/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalGraphPipeline.h

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
// .NAME vtkHierarchicalGraphPipeline - helper class for rendering graphs superimposed on a tree.
//
// .SECTION Description
// vtkHierarchicalGraphPipeline renders bundled edges that are meant to be
// viewed as an overlay on a tree. This class is not for general use, but
// is used in the internals of vtkRenderedHierarchyRepresentation and
// vtkRenderedTreeAreaRepresentation.

#include "vtkObject.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkApplyColors;
class vtkDataRepresentation;
class vtkDynamic2DLabelMapper;
class vtkEdgeCenters;
class vtkGraphHierarchicalBundleEdges;
class vtkGraphToPolyData;
class vtkPolyDataMapper;
class vtkRenderView;
class vtkSplineGraphEdges;
class vtkSelection;
class vtkTextProperty;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkHierarchicalGraphPipeline : public vtkObject
{
public:
  static vtkHierarchicalGraphPipeline* New();
  vtkTypeMacro(vtkHierarchicalGraphPipeline, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The actor associated with the hierarchical graph.
  vtkGetObjectMacro(Actor, vtkActor);

  // Description:
  // The actor associated with the hierarchical graph.
  vtkGetObjectMacro(LabelActor, vtkActor2D);

  // Description:
  // The bundling strength for the bundled edges.
  virtual void SetBundlingStrength(double strength);
  virtual double GetBundlingStrength();

  // Description:
  // The edge label array name.
  virtual void SetLabelArrayName(const char* name);
  virtual const char* GetLabelArrayName();

  // Description:
  // The edge label visibility.
  virtual void SetLabelVisibility(bool vis);
  virtual bool GetLabelVisibility();
  vtkBooleanMacro(LabelVisibility, bool);

  // Description:
  // The edge label text property.
  virtual void SetLabelTextProperty(vtkTextProperty* prop);
  virtual vtkTextProperty* GetLabelTextProperty();

  // Description:
  // The edge color array.
  virtual void SetColorArrayName(const char* name);
  virtual const char* GetColorArrayName();

  // Description:
  // Whether to color the edges by an array.
  virtual void SetColorEdgesByArray(bool vis);
  virtual bool GetColorEdgesByArray();
  vtkBooleanMacro(ColorEdgesByArray, bool);

  // Description:
  // The visibility of this graph.
  virtual void SetVisibility(bool vis);
  virtual bool GetVisibility();
  vtkBooleanMacro(Visibility, bool);

  // Description:
  // Returns a new selection relevant to this graph based on an input
  // selection and the view that this graph is contained in.
  virtual vtkSelection* ConvertSelection(vtkDataRepresentation* rep, vtkSelection* sel);

  // Description:
  // Sets the input connections for this graph.
  // graphConn is the input graph connection.
  // treeConn is the input tree connection.
  // annConn is the annotation link connection.
  virtual void PrepareInputConnections(
    vtkAlgorithmOutput* graphConn,
    vtkAlgorithmOutput* treeConn,
    vtkAlgorithmOutput* annConn);

  // Description:
  // Applies the view theme to this graph.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // The array to use while hovering over an edge.
  vtkSetStringMacro(HoverArrayName);
  vtkGetStringMacro(HoverArrayName);

  // Description:
  // The spline mode to use in vtkSplineGraphEdges.
  // vtkSplineGraphEdges::CUSTOM uses a vtkCardinalSpline.
  // vtkSplineGraphEdges::BSPLINE uses a b-spline.
  // The default is CUSTOM.
  virtual void SetSplineType(int type);
  virtual int GetSplineType();

  // Description:
  // Register progress with a view.
  void RegisterProgress(vtkRenderView* view);

protected:
  vtkHierarchicalGraphPipeline();
  ~vtkHierarchicalGraphPipeline();

  vtkApplyColors*                  ApplyColors;
  vtkGraphHierarchicalBundleEdges* Bundle;
  vtkGraphToPolyData*              GraphToPoly;
  vtkSplineGraphEdges*             Spline;
  vtkPolyDataMapper*               Mapper;
  vtkActor*                        Actor;
  vtkTextProperty*                 TextProperty;
  vtkEdgeCenters*                  EdgeCenters;
  vtkDynamic2DLabelMapper*         LabelMapper;
  vtkActor2D*                      LabelActor;

  char* HoverArrayName;

  vtkSetStringMacro(ColorArrayNameInternal);
  vtkGetStringMacro(ColorArrayNameInternal);
  char* ColorArrayNameInternal;

  vtkSetStringMacro(LabelArrayNameInternal);
  vtkGetStringMacro(LabelArrayNameInternal);
  char* LabelArrayNameInternal;

private:
  vtkHierarchicalGraphPipeline(const vtkHierarchicalGraphPipeline&); // Not implemented
  void operator=(const vtkHierarchicalGraphPipeline&); // Not implemented
};

