/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedHierarchyRepresentation.h

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
// .NAME vtkRenderedHierarchyRepresentation -
//
// .SECTION Description

#ifndef vtkRenderedHierarchyRepresentation_h
#define vtkRenderedHierarchyRepresentation_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkRenderedGraphRepresentation.h"

class VTKVIEWSINFOVIS_EXPORT vtkRenderedHierarchyRepresentation : public vtkRenderedGraphRepresentation
{
public:
  static vtkRenderedHierarchyRepresentation* New();
  vtkTypeMacro(vtkRenderedHierarchyRepresentation, vtkRenderedGraphRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  //
  virtual void SetGraphEdgeLabelArrayName(const char* name)
    { this->SetGraphEdgeLabelArrayName(name, 0); }
  virtual void SetGraphEdgeLabelArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeLabelArrayName()
    { return this->GetGraphEdgeLabelArrayName(0); }
  virtual const char* GetGraphEdgeLabelArrayName(int idx);

  virtual void SetGraphEdgeLabelVisibility(bool vis)
    { this->SetGraphEdgeLabelVisibility(vis, 0); }
  virtual void SetGraphEdgeLabelVisibility(bool vis, int idx);
  virtual bool GetGraphEdgeLabelVisibility()
    { return this->GetGraphEdgeLabelVisibility(0); }
  virtual bool GetGraphEdgeLabelVisibility(int idx);
  vtkBooleanMacro(GraphEdgeLabelVisibility, bool);

  virtual void SetGraphEdgeColorArrayName(const char* name)
    { this->SetGraphEdgeColorArrayName(name, 0); }
  virtual void SetGraphEdgeColorArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeColorArrayName()
    { return this->GetGraphEdgeColorArrayName(0); }
  virtual const char* GetGraphEdgeColorArrayName(int idx);

  virtual void SetColorGraphEdgesByArray(bool vis)
    { this->SetColorGraphEdgesByArray(vis, 0); }
  virtual void SetColorGraphEdgesByArray(bool vis, int idx);
  virtual bool GetColorGraphEdgesByArray()
    { return this->GetColorGraphEdgesByArray(0); }
  virtual bool GetColorGraphEdgesByArray(int idx);
  vtkBooleanMacro(ColorGraphEdgesByArray, bool);

  virtual void SetGraphEdgeColorToSplineFraction()
    { this->SetGraphEdgeColorArrayName("fraction", 0); }
  virtual void SetGraphEdgeColorToSplineFraction(int idx)
    { this->SetGraphEdgeColorArrayName("fraction", idx); }

  virtual void SetGraphVisibility(bool vis)
    { this->SetGraphVisibility(vis, 0); }
  virtual void SetGraphVisibility(bool vis, int idx);
  virtual bool GetGraphVisibility()
    { return this->GetGraphVisibility(0); }
  virtual bool GetGraphVisibility(int idx);
  vtkBooleanMacro(GraphVisibility, bool);

  virtual void SetBundlingStrength(double strength)
    { this->SetBundlingStrength(strength, 0); }
  virtual void SetBundlingStrength(double strength, int idx);
  virtual double GetBundlingStrength()
    { return this->GetBundlingStrength(0); }
  virtual double GetBundlingStrength(int idx);

  // Description:
  // Sets the spline type for the graph edges.
  // vtkSplineGraphEdges::CUSTOM uses a vtkCardinalSpline.
  // vtkSplineGraphEdges::BSPLINE uses a b-spline.
  // The default is BSPLINE.
  virtual void SetGraphSplineType(int type, int idx);
  virtual int GetGraphSplineType(int idx);

  virtual void SetGraphEdgeLabelFontSize(int size)
    { this->SetGraphEdgeLabelFontSize(size, 0); }
  virtual void SetGraphEdgeLabelFontSize(int size, int idx);
  virtual int GetGraphEdgeLabelFontSize()
    { return this->GetGraphEdgeLabelFontSize(0); }
  virtual int GetGraphEdgeLabelFontSize(int idx);

protected:
  vtkRenderedHierarchyRepresentation();
  ~vtkRenderedHierarchyRepresentation();

  // Description:
  // Called by the view to add/remove this representation.
  virtual bool AddToView(vtkView* view);
  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Whether idx is a valid graph index.
  bool ValidIndex(int idx);

  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* sel);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Sets up the input connections for this representation.
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual void ApplyViewTheme(vtkViewTheme* theme);

  //BTX
  class Internals;
  Internals* Implementation;
  //ETX

private:
  vtkRenderedHierarchyRepresentation(const vtkRenderedHierarchyRepresentation&); // Not implemented
  void operator=(const vtkRenderedHierarchyRepresentation&);   // Not implemented
};

#endif

