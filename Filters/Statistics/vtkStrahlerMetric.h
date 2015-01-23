/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStrahlerMetric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//-------------------------------------------------------------------------
//Copyright 2008 Sandia Corporation.
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
//-------------------------------------------------------------------------
//
// .NAME vtkStrahlerMetric - compute Strahler metric for a tree
// .SECTION Description
// The Strahler metric is a value assigned to each vertex of a
// tree that characterizes the structural complexity of the
// sub-tree rooted at that node.  The metric originated in the
// study of river systems, but has been applied to other tree-
// structured systes,  Details of the metric and the rationale
// for using it in infovis can be found in:
//
// Tree Visualization and Navigation Clues for Information
// Visualization, I. Herman, M. Delest, and G. Melancon,
// Computer Graphics Forum, Vol 17(2), Blackwell, 1998.
//
// The input tree is copied to the output, but with a new array
// added to the output vertex data.
//
// .SECTION Thanks
// Thanks to David Duke from the University of Leeds for providing this
// implementation.

#ifndef vtkStrahlerMetric_h
#define vtkStrahlerMetric_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class vtkFloatArray;

class VTKFILTERSSTATISTICS_EXPORT vtkStrahlerMetric : public vtkTreeAlgorithm
{
public:
  static vtkStrahlerMetric *New();
  vtkTypeMacro(vtkStrahlerMetric,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the array in which the Strahler values will
  // be stored within the output vertex data.
  // Default is "Strahler"
  vtkSetStringMacro(MetricArrayName);

  // Description:
  // Set/get setting of normalize flag.  If this is set, the
  // Strahler values are scaled into the range [0..1].
  // Default is for normalization to be OFF.
  vtkSetMacro(Normalize, int);
  vtkGetMacro(Normalize, int);
  vtkBooleanMacro(Normalize, int);

  // Description:
  // Get the maximum strahler value for the tree.
  vtkGetMacro(MaxStrahler,float);

protected:
  vtkStrahlerMetric();
  ~vtkStrahlerMetric();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int Normalize;
  float MaxStrahler;
  char *MetricArrayName;

  float CalculateStrahler(vtkIdType root, vtkFloatArray *metric, vtkTree *graph);

private:
  vtkStrahlerMetric(const vtkStrahlerMetric&);  // Not implemented.
  void operator=(const vtkStrahlerMetric&);  // Not implemented.
};

#endif

