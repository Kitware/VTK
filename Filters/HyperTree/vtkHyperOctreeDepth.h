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
/**
 * @class   vtkHyperOctreeDepth
 * @brief   Assign tree depth attribute to each cell.
 *
 * This filter returns a shallow copy of its input HyperOctree with a new
 * data attribute field containing the depth of each cell.
 *
 * @sa
 * vtkHyperOctree
*/

#ifndef vtkHyperOctreeDepth_h
#define vtkHyperOctreeDepth_h

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
  ~vtkHyperOctreeDepth() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  vtkHyperOctree *Input;
  vtkHyperOctree *Output;

  void TraverseAndCount(vtkHyperOctreeCursor *, int depth);

  vtkIntArray *GeneratedDepths;
  int NumChildren;

private:
  vtkHyperOctreeDepth(const vtkHyperOctreeDepth&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperOctreeDepth&) VTK_DELETE_FUNCTION;
};

#endif
