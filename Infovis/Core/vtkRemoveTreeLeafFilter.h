/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveTreeLeafFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRemoveTreeLeafFilter - remove leaves from a vtkTree
//
// .SECTION Description
// Removes specified leaves from a vtkTree. User can choose whether to remove the parent
// node whose children are removed via "ShouldRemoveParentVertex", default to be true.
// Two inputs:
// input 0 --- vtkTree
// input 1 --- vtkSelection  (contains the list of leaf vertices to be removed)

#ifndef __vtkRemoveTreeLeafFilter_h
#define __vtkRemoveTreeLeafFilter_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class vtkTree;
class vtkIdTypeArray;
class vtkMutableDirectedGraph;

class VTKINFOVISCORE_EXPORT vtkRemoveTreeLeafFilter : public vtkTreeAlgorithm
{
public:
  static vtkRemoveTreeLeafFilter* New();
  vtkTypeMacro(vtkRemoveTreeLeafFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Should we remove the parent vertex if the parent vertex has no other
  // children besides the removed children.
  // Default behavior is to remove the parent vertex.
  vtkGetMacro(ShouldRemoveParentVertex, bool);
  vtkSetMacro(ShouldRemoveParentVertex, bool);

protected:
  vtkRemoveTreeLeafFilter();
  ~vtkRemoveTreeLeafFilter();

  int FillInputPortInformation(int port, vtkInformation* info);
  bool ShouldRemoveParentVertex;
  int BuildTree(vtkMutableDirectedGraph * builder, vtkIdType parentId, vtkTree * inputTree,
                vtkIdType inputTreeVertexId, vtkIdTypeArray * list);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkRemoveTreeLeafFilter(const vtkRemoveTreeLeafFilter&); // Not implemented
  void operator=(const vtkRemoveTreeLeafFilter&);   // Not implemented
};

#endif
