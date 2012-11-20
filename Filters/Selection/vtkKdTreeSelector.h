/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdTreeSelector.h

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
// .NAME vtkKdTreeSelector - Selects point ids using a kd-tree.
//
// .SECTION Description
// If SetKdTree is used, the filter ignores the input and selects based on that
// kd-tree.  If SetKdTree is not used, the filter builds a kd-tree using the
// input point set and uses that tree for selection.  The output is a
// vtkSelection containing the ids found in the kd-tree using the specified
// bounds.

#ifndef __vtkKdTreeSelector_h
#define __vtkKdTreeSelector_h

#include "vtkFiltersSelectionModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

class vtkKdTree;

class VTKFILTERSSELECTION_EXPORT vtkKdTreeSelector : public vtkSelectionAlgorithm
{
public:
  static vtkKdTreeSelector* New();
  vtkTypeMacro(vtkKdTreeSelector, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The kd-tree to use to find selected ids.
  // The kd-tree must be initialized with the desired set of points.
  // When this is set, the optional input is ignored.
  void SetKdTree(vtkKdTree* tree);
  vtkGetObjectMacro(KdTree, vtkKdTree);

  // Description:
  // The bounds of the form (xmin,xmax,ymin,ymax,zmin,zmax).
  // To perform a search in 2D, use the bounds
  // (xmin,xmax,ymin,ymax,VTK_DOUBLE_MIN,VTK_DOUBLE_MAX).
  vtkSetVector6Macro(SelectionBounds, double);
  vtkGetVector6Macro(SelectionBounds, double);

  // Description:
  // The field name to use when generating the selection.
  // If set, creates a VALUES selection.
  // If not set (or is set to NULL), creates a INDICES selection.
  // By default this is not set.
  vtkSetStringMacro(SelectionFieldName);
  vtkGetStringMacro(SelectionFieldName);

  // Description:
  // The field attribute to use when generating the selection.
  // If set, creates a PEDIGREEIDS or GLOBALIDS selection.
  // If not set (or is set to -1), creates a INDICES selection.
  // By default this is not set.
  // NOTE: This should be set a constant in vtkDataSetAttributes,
  // not vtkSelection.
  vtkSetMacro(SelectionAttribute, int);
  vtkGetMacro(SelectionAttribute, int);

  // Description:
  // Whether to only allow up to one value in the result.
  // The item selected is closest to the center of the bounds,
  // if there are any points within the selection threshold.
  // Default is off.
  vtkSetMacro(SingleSelection, bool);
  vtkGetMacro(SingleSelection, bool);
  vtkBooleanMacro(SingleSelection, bool);

  // Description:
  // The threshold for the single selection.
  // A single point is added to the selection if it is within
  // this threshold from the bounds center.
  // Default is 1.
  vtkSetMacro(SingleSelectionThreshold, double);
  vtkGetMacro(SingleSelectionThreshold, double);

  unsigned long GetMTime();

protected:
  vtkKdTreeSelector();
  ~vtkKdTreeSelector();

  vtkKdTree* KdTree;
  double SelectionBounds[6];
  char* SelectionFieldName;
  bool BuildKdTreeFromInput;
  bool SingleSelection;
  double SingleSelectionThreshold;
  int SelectionAttribute;

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkKdTreeSelector(const vtkKdTreeSelector&); // Not implemented
  void operator=(const vtkKdTreeSelector&);   // Not implemented
};

#endif
