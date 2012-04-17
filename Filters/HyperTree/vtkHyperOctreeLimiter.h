/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeLimiter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeLimiter - Limit the tree's depth, averaging data
//  from lower level branches into the new leaves at the cut points.
// .SECTION Description
// This filter returns a lower resolution copy of its input vtkHyperOctree.
// It does a length/area/volume weighted averaging to obtain data at each
// cut point. Above the cut level, leaf attribute data is simply copied.

// .SECTION See Also
// vtkHyperOctree

#ifndef __vtkHyperOctreeLimiter_h
#define __vtkHyperOctreeLimiter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkHyperOctree;
class vtkHyperOctreeCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeLimiter : public vtkDataSetAlgorithm
{
public:
  static vtkHyperOctreeLimiter *New();
  vtkTypeMacro(vtkHyperOctreeLimiter, vtkDataSetAlgorithm);

  // Description:
  // Return the maximum number of levels of the hyperoctree.
  int GetMaximumLevel();

  // Description:
  // Set the maximum number of levels of the hyperoctree.
  void SetMaximumLevel(int levels);

protected:
  vtkHyperOctreeLimiter();
  ~vtkHyperOctreeLimiter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  vtkHyperOctree *Input;
  vtkHyperOctree *Output;

  void BuildNextCell(vtkHyperOctreeCursor *, vtkHyperOctreeCursor *, int);

  void AddInteriorAttributes(vtkHyperOctreeCursor *, int);
  double MeasureCell(int);

  int MaximumLevel;
  double TopSize;
  int Dimension;
  int NumChildren;
  double SizeAtPrunePoint;

  double *AccumScratch;
  int AccumSize;

private:
  vtkHyperOctreeLimiter(const vtkHyperOctreeLimiter&);  // Not implemented.
  void operator=(const vtkHyperOctreeLimiter&);  // Not implemented.
};

#endif
