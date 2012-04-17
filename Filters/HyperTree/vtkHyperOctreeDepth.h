/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeDepth.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeDepth - Assign tree depth attribute to each cell.
// .SECTION Description
// This filter returns a shallow copy of its input HyperOctree with a new
// data attribute field containing the depth of each cell.

// .SECTION See Also
// vtkHyperOctree

#ifndef __vtkHyperOctreeDepth_h
#define __vtkHyperOctreeDepth_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkHyperOctree;
class vtkHyperOctreeCursor;
class vtkIntArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeDepth : public vtkDataSetAlgorithm
{
public:
  static vtkHyperOctreeDepth *New();
  vtkTypeMacro(vtkHyperOctreeDepth, vtkDataSetAlgorithm);

protected:
  vtkHyperOctreeDepth();
  ~vtkHyperOctreeDepth();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  vtkHyperOctree *Input;
  vtkHyperOctree *Output;

  void TraverseAndCount(vtkHyperOctreeCursor *, int depth);

  vtkIntArray *GeneratedDepths;
  int NumChildren;

private:
  vtkHyperOctreeDepth(const vtkHyperOctreeDepth&);  // Not implemented.
  void operator=(const vtkHyperOctreeDepth&);  // Not implemented.
};

#endif
